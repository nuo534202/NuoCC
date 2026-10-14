#include "nuocc_asm_codegen.hpp"

#include <stdlib.h>

#include <iostream>
#include <variant>
#include <vector>

#include "utils/nuocc_type_check.hpp"

namespace nuocc
{

AsmCodegen::AsmCodegen(const std::string& output_file)
    : next_label_(1),
      function_end_label_(0),
      function_return_type_(PrimitiveType::kNone),
      function_local_size_(0),
      ofs_(output_file, std::ios::out | std::ios::trunc)
{
    if (!ofs_.is_open())
    {
        std::cerr << "Error: cannot open output file!" << std::endl;
        std::exit(1);
    }

    FreeAllRegister();
}

AsmCodegen::~AsmCodegen()
{
    ofs_.close();
}

void AsmCodegen::GenProgram(const Program& program)
{
    FreeAllRegister();

    EmitPreamble();

    /*
     * The storage for the global variables is emitted before any code, so
     * that a function can use a variable declared below it.
     */
    for (const AstNodePtr& declaration : program.globals)
    {
        const AstDeclare *glob =
            static_cast<const AstDeclare*>(declaration.get());

        GenGlobSymbol(glob->GetSymbol());
    }

    for (const AstNodePtr& function : program.functions)
    {
        const AstFunction *ast_function =
            static_cast<const AstFunction*>(function.get());

        /*
         * Every return statement jumps here, and the type tells the target
         * where the result of a return has to end up.
         */
        function_end_label_ = NewLabel();
        function_return_type_ = ast_function->GetSymbol().type;
        function_local_size_ = ast_function->GetLocalSize();

        EmitFunctionPreamble(ast_function->GetSymbol(),
                             function_local_size_,
                             ast_function->GetParameters());

        GenStatement(function->GetLeft());

        FreeAllRegister();

        EmitFunctionPostamble();

        function_end_label_ = 0;
        function_return_type_ = PrimitiveType::kNone;
        function_local_size_ = 0;
    }
}

/*
 * Write the start of a global variable's storage, which everything a global
 * holds is written after.
 */
void AsmCodegen::EmitGlobHeading(const Symbol& symbol)
{
    ofs_ << "\t.data" << std::endl;

    /*
     * The assembler is told which name other files may reach, and left to
     * itself about one declared static, which only this file may.
     */
    if (!symbol.is_static)
        ofs_ << "\t.globl\t" << GlobName(symbol.name) << std::endl;

    /*
     * The storage is lined up to what it holds, so that every element of it
     * lands where its type says and so that a pointer an element holds can
     * be written into it. The data section of some machines lines a pointer
     * up no further than this whatever the assembler is told, which is why
     * the alignment is asked for here rather than left to them.
     */
    const int32 alignment = SymbolAlignment(symbol);

    if (alignment > 1)
        ofs_ << "\t.balign\t" << alignment << std::endl;

    ofs_ << GlobName(symbol.name) << ":" << std::endl;
}

/*
 * Generate the code for one statement. A statement yields no value, so
 * nothing is returned. A null node is an empty compound statement.
 */
void AsmCodegen::GenStatement(const AstNodePtr& root)
{
    if (!root)
        return;

    if (root->GetType() == PrimitiveType::kComposite) {
        std::cerr << "syntax error: whole composite values are not supported yet!"
                  << std::endl;
        std::exit(1);
    }

    switch (root->GetAstNodeTag())
    {
        case A_AstGlue:
            /* The statements were glued left to right, so run them in order. */
            GenStatement(root->GetLeft());
            FreeAllRegister();
            GenStatement(root->GetRight());
            FreeAllRegister();
            return;

        case A_AstPrint:
        {
            reg_idx reg = GenExpr(root->GetLeft());
            PrintInt(reg);
            FreeRegister(reg);
            return;
        }

        case A_AstDeclare:
            return;

        case A_AstIf:
            GenIf(root);
            return;

        case A_AstWhile:
            GenWhile(root);
            return;

        case A_AstFor:
            GenFor(root);
            return;

        case A_AstSwitch:
            GenSwitch(root);
            return;

        case A_AstBreak:
            /* Leave the loop this statement stands in. */
            EmitJump(loop_break_label_);
            return;

        case A_AstContinue:
            /* Begin the next pass of that loop. */
            EmitJump(loop_continue_label_);
            return;

        case A_AstReturn:
        {
            reg_idx reg = GenExpr(root->GetLeft());
            Return(reg);
            FreeRegister(reg);
            return;
        }

        /*
         * An expression standing on its own, which is how an assignment or
         * an increment becomes a statement, and how the result of a call
         * comes to be discarded: the value is worked out and then thrown
         * away.
         */
        case A_AstOperator:
        case A_AstFuncCall:
        case A_AstIncDec:
        case A_AstMember:
        case A_AstDeref:
        case A_AstIdentifier:
        case A_AstCast:
        case A_AstAddressExpr:
        {
            reg_idx reg = GenExpr(root);
            FreeRegister(reg);
            return;
        }

        default:
            break;
    }

    std::cerr << "Error: node " << root->GetAstNodeTag();
    std::cerr << " is not a statement!" << std::endl;
    std::exit(1);
}

/*
 * Generate the code for an if statement:
 *
 *      <condition>, jumping to Lfalse when it does not hold
 *      <true branch>
 *      jmp Lend
 * Lfalse:
 *      <else branch>
 * Lend:
 *
 * Without an else clause the false label is also the end label, so the
 * code simply falls through to it.
 */
void AsmCodegen::GenIf(const AstNodePtr& root)
{
    const AstIf *ast_if = static_cast<const AstIf*>(root.get());

    label_idx false_label = NewLabel();
    label_idx end_label = false_label;

    if (ast_if->HasElse())
        end_label = NewLabel();

    GenCondition(root->GetLeft(), false_label);
    FreeAllRegister();

    GenStatement(root->GetMid());
    FreeAllRegister();

    if (ast_if->HasElse())
        EmitJump(end_label);

    EmitLabel(false_label);

    if (ast_if->HasElse())
    {
        GenStatement(root->GetRight());
        FreeAllRegister();
        EmitLabel(end_label);
    }
}

/*
 * Generate the code for a while loop:
 *
 * Lstart:
 *      <condition>, jumping to Lend when it does not hold
 *      <body>
 *      jmp Lstart
 * Lend:
 *
 * A continue of this loop goes back to Lstart, since there is nowhere else
 * for the next pass to begin.
 */
void AsmCodegen::GenWhile(const AstNodePtr& root)
{
    label_idx start_label = NewLabel();
    label_idx end_label = NewLabel();

    const label_idx saved_break = loop_break_label_;
    const label_idx saved_continue = loop_continue_label_;
    loop_break_label_ = end_label;
    loop_continue_label_ = start_label;

    EmitLabel(start_label);

    GenCondition(root->GetLeft(), end_label);
    FreeAllRegister();

    GenStatement(root->GetRight());
    FreeAllRegister();

    EmitJump(start_label);
    EmitLabel(end_label);

    loop_break_label_ = saved_break;
    loop_continue_label_ = saved_continue;
}

/*
 * Generate the code for a for loop, which is a while loop with a step at
 * the end of every pass:
 *
 * Lstart:
 *      <condition>, jumping to Lend when it does not hold
 *      <body>
 * Lnext:
 *      <step>
 *      jmp Lstart
 * Lend:
 *
 * A continue of this loop goes to Lnext and not to Lstart, so that the
 * step still runs on the pass the continue began.
 */
void AsmCodegen::GenFor(const AstNodePtr& root)
{
    label_idx start_label = NewLabel();
    label_idx end_label = NewLabel();

    /*
     * The step is where a continue goes and so needs a label of its own. A
     * loop written without one has nothing to skip to, which leaves the
     * next pass beginning where the condition does.
     */
    label_idx next_label = start_label;

    if (root->GetRight())
        next_label = NewLabel();

    const label_idx saved_break = loop_break_label_;
    const label_idx saved_continue = loop_continue_label_;
    loop_break_label_ = end_label;
    loop_continue_label_ = next_label;

    EmitLabel(start_label);

    GenCondition(root->GetLeft(), end_label);
    FreeAllRegister();

    GenStatement(root->GetMid());
    FreeAllRegister();

    if (root->GetRight())
    {
        EmitLabel(next_label);
        GenStatement(root->GetRight());
        FreeAllRegister();
    }

    EmitJump(start_label);
    EmitLabel(end_label);

    loop_break_label_ = saved_break;
    loop_continue_label_ = saved_continue;
}

/*
 * Generate the code for a switch statement:
 *
 *      <expression>
 *      compare against the value of every case, going to the arm which
 *      matches
 *      jmp Ldefault
 * Lcase0:
 *      <first arm>
 * Lcase1:
 *      <second arm>
 * ...
 * Ldefault:
 *      <default arm, when there is one>
 * Lend:
 *
 * The arms are laid out one after another, so an arm which does not end in
 * a break falls through into the arm after it. A break of the switch goes
 * to Lend, which is where Ldefault goes too when no arm matched and there
 * is no default arm at all.
 */
void AsmCodegen::GenSwitch(const AstNodePtr& root)
{
    const AstSwitch *ast_switch = static_cast<const AstSwitch*>(root.get());
    const std::vector<AstNodePtr>& cases = ast_switch->GetCases();

    /* The value every arm is compared against, worked out once. */
    reg_idx reg = GenExpr(root->GetLeft());

    const label_idx end_label = NewLabel();

    /* One label per arm, in the order the arms are written. */
    std::vector<label_idx> labels;
    label_idx default_label = end_label;

    for (const AstNodePtr& arm : cases)
    {
        const AstCase *ast_case = static_cast<const AstCase*>(arm.get());

        labels.push_back(NewLabel());

        if (ast_case->IsDefault())
            default_label = labels.back();
    }

    for (idx_t k = 0; k < cases.size(); k++)
    {
        const AstCase *ast_case = static_cast<const AstCase*>(cases[k].get());

        if (ast_case->IsDefault())
            continue;

        reg_idx case_reg = LoadInt(ast_case->GetValue());
        JumpIfEqual(reg, case_reg, labels[k]);
        FreeRegister(case_reg);
    }

    FreeRegister(reg);
    EmitJump(default_label);

    /*
     * The arms follow one another, so that the end of one falls through
     * into the next. A break written in an arm leaves the switch, and a
     * continue written in one still belongs to the loop the switch is in.
     */
    const label_idx saved_break = loop_break_label_;
    loop_break_label_ = end_label;

    for (idx_t k = 0; k < cases.size(); k++)
    {
        EmitLabel(labels[k]);
        GenStatement(cases[k]->GetLeft());
        FreeAllRegister();
    }

    loop_break_label_ = saved_break;

    EmitLabel(end_label);
}

/*
 * Generate the code for an if or a while condition. The condition has to be
 * a comparison, so it becomes a compare followed by a jump to the false
 * label when the comparison does not hold.
 */
void AsmCodegen::GenCondition(const AstNodePtr& condition,
    label_idx false_label)
{
    /*
     * A comparison compares its two operands and jumps when it does not
     * hold. Any other condition has been turned into a boolean value, so
     * all that is left is to ask whether that value is zero.
     */
    if (condition->GetAstNodeTag() == A_AstOperator)
    {
        const AstOperator *ast_op =
            static_cast<const AstOperator*>(condition.get());

        reg_idx left_reg = GenExpr(condition->GetLeft());
        reg_idx right_reg = GenExpr(condition->GetRight());

        CompareAndJump(ast_op->GetOpType(), left_reg, right_reg, false_label);
        return;
    }

    reg_idx reg = GenExpr(condition);

    JumpIfZero(reg, false_label);
}

/*
 * Reserve the storage of a global variable.
 *
 * The variables are laid out one after another in the data section, in the
 * order they are declared, so that a program can reach one by adding an
 * offset to the address of another. The code section is restored after
 * each global declaration.
 */
void AsmCodegen::GenGlobSymbol(const Symbol& symbol)
{
    /*
     * A struct or a union is one block of room which starts out zero: it
     * cannot be given a value where it is declared.
     */
    if (symbol.type == PrimitiveType::kComposite)
    {
        EmitGlobHeading(symbol);
        ofs_ << "\t.space\t" << SymbolStorageSize(symbol) << std::endl;
        ofs_ << "\t.text" << std::endl;
        return;
    }

    /*
     * A variable holds one element unless it is an array, which holds as
     * many elements as it was declared with.
     */
    const bool is_array = symbol.stype == StructuralType::kArray;
    const int32 element_count = is_array ? symbol.element_count : 1;
    const int32 element_size = ValueSize(symbol.type, symbol.composite);

    /*
     * An element given a string holds the address of a string, so that
     * string needs room of its own. Every one of them is given a label
     * before the variable is written, so that the address an element holds
     * is known by the time it is.
     */
    std::vector<label_idx> string_labels(element_count, 0);

    for (idx_t k = 0;
         k < string_labels.size() && k < symbol.initial_values.size();
         k++)
    {
        if (const std::string* text =
                std::get_if<std::string>(&symbol.initial_values[k]))
        {
            string_labels[k] = NewLabel();
            GenStrStorage(string_labels[k], *text);
        }
    }

    EmitGlobHeading(symbol);

    for (int32 k = 0; k < element_count; k++)
    {
        /* An element given a string holds the address of that string. */
        if (string_labels[k] != 0)
        {
            ofs_ << "\t.quad\tL" << string_labels[k] << std::endl;
            continue;
        }

        /*
         * An element which was not given a value starts out zero, which is
         * what an array with fewer values than elements needs.
         */
        int32 value = 0;

        if (k < static_cast<int32>(symbol.initial_values.size()))
            value = std::get<int32>(symbol.initial_values[k]);

        switch (element_size)
        {
            case 1:
                /*
                 * A value may be wider than the element holding it, as a
                 * cast in front of it says, and then the element keeps the
                 * low bits of it, the way a store does.
                 */
                ofs_ << "\t.byte\t" << (value & 0xff) << std::endl;
                break;
            case 4:
                ofs_ << "\t.long\t" << value << std::endl;
                break;
            case 8:
                ofs_ << "\t.quad\t" << value << std::endl;
                break;
            default:
                std::cerr << "Error: bad type for variable ";
                std::cerr << symbol.name << "!" << std::endl;
                std::exit(1);
        }
    }

    ofs_ << "\t.text" << std::endl;
}

/*
 * The storage holds one byte per character and then the NUL which ends
 * the string, all under a local label: unlike a global variable, a
 * literal has no name the rest of the program could refer to.
 */
void AsmCodegen::GenStrStorage(label_idx label, const std::string& text)
{
    ofs_ << "\t.data" << std::endl;
    EmitLabel(label);

    ofs_ << "\t.byte\t";

    for (std::size_t k = 0; k < text.size(); k++)
    {
        if (k > 0)
            ofs_ << ", ";

        ofs_ << static_cast<int>(static_cast<unsigned char>(text[k]));
    }

    if (!text.empty())
        ofs_ << ", ";

    ofs_ << "0" << std::endl;
    ofs_ << "\t.text" << std::endl;
}

std::string AsmCodegen::GlobName(const std::string& name) const
{
    return name;
}

/*
 * Generate the code for an expression and return the register which holds
 * its value. The caller owns that register and has to free it.
 */
reg_idx AsmCodegen::GenExpr(const AstNodePtr& root)
{
    if (root->GetType() == PrimitiveType::kComposite) {
        std::cerr << "syntax error: whole composite values are not supported yet!"
                  << std::endl;
        std::exit(1);
    }
    switch (root->GetAstNodeTag())
    {
        case A_AstIntLit:
        {
            const AstIntLit *int_lit =
                static_cast<const AstIntLit*>(root.get());
            return LoadInt(int_lit->GetValue());
        }
        case A_AstCast:
        {
            reg_idx reg = GenExpr(root->GetLeft());

            return Cast(reg, root->GetLeft()->GetType(), root->GetType());
        }
        case A_AstStrLit:
        {
            const AstStrLit *str_lit =
                static_cast<const AstStrLit*>(root.get());

            label_idx label = NewLabel();
            GenStrStorage(label, str_lit->GetText());

            return LoadStrAddress(label);
        }
        case A_AstIdentifier:
        {
            const AstIdentifier *ident =
                static_cast<const AstIdentifier*>(root.get());

            return LoadSymbol(ident->GetSymbol());
        }
        case A_AstWiden:
        {
            reg_idx reg = GenExpr(root->GetLeft());

            return Widen(reg, root->GetLeft()->GetType(), root->GetType());
        }
        case A_AstScale:
        {
            const AstScale *scale =
                static_cast<const AstScale*>(root.get());

            reg_idx reg = GenExpr(root->GetLeft());

            return Scale(reg, scale->GetSize());
        }
        case A_AstAddress:
        {
            const AstAddress *address =
                static_cast<const AstAddress*>(root.get());
            return AddressOf(address->GetSymbol());
        }
        case A_AstDeref:
        {
            reg_idx reg = GenExpr(root->GetLeft());

            return Deref(reg, root->GetLeft()->GetType());
        }
        case A_AstMember:
        {
            const auto& member = static_cast<const AstMember&>(*root);
            const reg_idx reg = GenAddress(root);
            if (member.GetMember().stype == StructuralType::kArray)
                return reg;
            return Deref(reg, PointerTo(root->GetType()));
        }
        case A_AstAddressExpr:
            return GenAddress(root->GetLeft());
        /*
         * A unary operation works on the value its child leaves in a
         * register, and the tag of the node says which one it is.
         */
        case A_AstNegate:
            return Negate(GenExpr(root->GetLeft()));
        case A_AstInvert:
            return Invert(GenExpr(root->GetLeft()));
        case A_AstLogNot:
            return LogNot(GenExpr(root->GetLeft()));
        case A_AstToBool:
            return ToBool(GenExpr(root->GetLeft()));
        case A_AstIncDec:
        {
            const AstIncDec *inc =
                static_cast<const AstIncDec*>(root.get());

            return IncDec(inc->GetSymbol(), inc->GetDelta(), inc->IsPost());
        }
        case A_AstFuncCall:
            return GenCall(root);
        case A_AstOperator:
        {
            const AstOperator *ast_op =
                static_cast<const AstOperator*>(root.get());
            NodeTag op_type = ast_op->GetOpType();

            if (op_type == T_Assign)
                return GenAssign(root);

            /*
             * A compound assignment combines the value of the target with
             * the value on the right before storing the result, so it is
             * not an operation on the two children the way every other
             * operator is.
             */
            if (IsCompoundAssign(op_type))
                return GenAssignOp(root, CompoundAssignOperator(op_type));

            reg_idx left_reg = GenExpr(root->GetLeft());
            reg_idx right_reg = GenExpr(root->GetRight());

            return GenOperator(op_type, left_reg, right_reg);
        }
        default:
            break;
    }

    std::cerr << "Error: node " << root->GetAstNodeTag();
    std::cerr << " is not an expression!" << std::endl;
    std::exit(1);
}

/*
 * Call a function. The arguments are worked out from the last one to the
 * first and pushed on the stack in that order, so that the first argument
 * ends up nearest to the stack pointer; the target takes them from there.
 * Working them out from the last one also means that an argument which
 * needs a function of its own does not disturb the ones already pushed.
 */
reg_idx AsmCodegen::GenCall(const AstNodePtr& root)
{
    const AstFuncCall *call = static_cast<const AstFuncCall*>(root.get());
    const std::vector<AstNodePtr>& arguments = call->GetArguments();

    for (auto it = arguments.rbegin(); it != arguments.rend(); ++it)
    {
        reg_idx arg_reg = GenExpr(*it);

        PushArgument(arg_reg);
        FreeRegister(arg_reg);
    }

    return Call(call->GetSymbol(), static_cast<int32>(arguments.size()));
}

/*
 * Generate the code for an assignment and return the register which holds
 * the value stored, so that an assignment can be used as an expression.
 *
 * The value is worked out first and the place it goes in second: the tree
 * holds the value in its left child and the place in its right one, the
 * two having been switched around when the '=' was parsed. The place is
 * never evaluated as a value of its own. A variable is stored to by name,
 * and a pointer only gives up the address it holds, which the value is
 * then written through.
 */
reg_idx AsmCodegen::GenAssign(const AstNodePtr& root)
{
    reg_idx value_reg = GenExpr(root->GetLeft());

    const AstNodePtr& target = root->GetRight();

    switch (target->GetAstNodeTag())
    {
        case A_AstIdentifier:
        {
            const AstIdentifier *ident =
                static_cast<const AstIdentifier*>(target.get());

            StoreSymbol(ident->GetSymbol(), value_reg);
            return value_reg;
        }
        case A_AstDeref:
        case A_AstMember:
        {
            reg_idx address_reg = GenAddress(target);

            StoreDeref(value_reg, address_reg, target->GetType());
            FreeRegister(address_reg);
            return value_reg;
        }
        default:
            break;
    }

    std::cerr << "Error: node " << target->GetAstNodeTag();
    std::cerr << " cannot be assigned to!" << std::endl;
    std::exit(1);
}

// Compute an lvalue's address without loading the value stored there.
/*
 * Generate the code for a compound assignment, whose target holds the value
 * it is combined with:
 *
 *      value = <right>
 *      current = <the value the target holds now>
 *      result = current <op> value
 *      <target> = result
 *
 * The address of the target is worked out once, so that an index or a call
 * written inside it runs once, the way it does for an assignment.
 */
reg_idx AsmCodegen::GenAssignOp(const AstNodePtr& root, NodeTag op_type)
{
    const AstNodePtr& target = root->GetRight();

    /* The value on the right is worked out first, so that nothing the
     * target needs while it is read can disturb it. */
    reg_idx value_reg = GenExpr(root->GetLeft());

    switch (target->GetAstNodeTag())
    {
        case A_AstIdentifier:
        {
            const AstIdentifier *ident =
                static_cast<const AstIdentifier*>(target.get());

            reg_idx current_reg = LoadSymbol(ident->GetSymbol());
            reg_idx result_reg = GenOperator(op_type, current_reg, value_reg);

            StoreSymbol(ident->GetSymbol(), result_reg);
            return result_reg;
        }

        case A_AstDeref:
        case A_AstMember:
        {
            reg_idx address_reg = GenAddress(target);
            reg_idx current_reg = LoadDeref(address_reg, target->GetType());
            reg_idx result_reg = GenOperator(op_type, current_reg, value_reg);

            StoreDeref(result_reg, address_reg, target->GetType());
            FreeRegister(address_reg);
            return result_reg;
        }

        default:
            break;
    }

    std::cerr << "Error: node " << target->GetAstNodeTag();
    std::cerr << " cannot be assigned to!" << std::endl;
    std::exit(1);
}

reg_idx AsmCodegen::GenAddress(const AstNodePtr& root) {
    switch (root->GetAstNodeTag()) {
        case A_AstIdentifier:
            return AddressOf(static_cast<const AstIdentifier&>(*root)
                                 .GetSymbol());
        case A_AstDeref:
            return GenExpr(root->GetLeft());
        case A_AstMember: {
            const auto& member = static_cast<const AstMember&>(*root);
            const reg_idx reg = member.IsThroughPointer()
                ? GenExpr(root->GetLeft()) : GenAddress(root->GetLeft());
            AddOffset(reg, member.GetMember().member_offset);
            return reg;
        }
        default:
            std::cerr << "Error: expression has no address!" << std::endl;
            std::exit(1);
    }
}

reg_idx AsmCodegen::GenOperator(NodeTag op_type,
    reg_idx left_reg,
    reg_idx right_reg)
{
    switch (op_type)
    {
        case T_Plus:
            return Add(left_reg, right_reg);
        case T_Minus:
            return Sub(left_reg, right_reg);
        case T_Star:
            return Mul(left_reg, right_reg);
        case T_Slash:
            return Div(left_reg, right_reg);
        case T_Amper:
            return And(left_reg, right_reg);
        case T_Or:
            return Or(left_reg, right_reg);
        case T_Xor:
            return Xor(left_reg, right_reg);
        case T_LShift:
            return ShiftLeft(left_reg, right_reg);
        case T_RShift:
            return ShiftRight(left_reg, right_reg);
        /*
         * The operands of '&&' and '||' have already been turned into the
         * zero or one which says whether they hold, so combining them is
         * the bitwise operation on those two values.
         */
        case T_LogAnd:
            return And(left_reg, right_reg);
        case T_LogOr:
            return Or(left_reg, right_reg);
        case T_EQ:
        case T_NE:
        case T_LT:
        case T_GT:
        case T_LE:
        case T_GE:
            return CompareAndSet(op_type, left_reg, right_reg);
        default:
            break;
    }

    std::cerr << "Error: token " << op_type;
    std::cerr << " is not a binary operator!" << std::endl;
    std::exit(1);
}

reg_idx AsmCodegen::AllocRegister()
{
    for (reg_idx i = 0; i < kRegSize; i++)
        if (is_free_[i])
        {
            is_free_[i] = false;
            return i;
        }

    std::cerr << "Error: out of registers!" << std::endl;
    std::exit(1);
}

void AsmCodegen::FreeRegister(reg_idx reg)
{
    if (is_free_[reg])
    {
        std::cerr << "Error: trying to free unused register ";
        std::cerr << reg << "!" << std::endl;
        std::exit(1);
    }

    is_free_[reg] = true;
}

void AsmCodegen::FreeAllRegister()
{
    for (idx_t i = 0; i < kRegSize; i++)
        is_free_[i] = true;
}

label_idx AsmCodegen::NewLabel()
{
    return next_label_++;
}

const bool* AsmCodegen::FreeRegisters() const
{
    return is_free_;
}

}   /* namespace nuocc */
