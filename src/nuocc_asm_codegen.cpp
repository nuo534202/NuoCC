#include "nuocc_asm_codegen.hpp"

#include <stdlib.h>

#include <iostream>
#include <variant>
#include <vector>

#include "utils/nuocc_type_check.hpp"

namespace nuocc
{

namespace
{

/*
 * The type which takes the given room on the machine this compiler is
 * built for, which is what a value of that room is moved as. Every room a
 * copy moves a piece of is the room of one of the language's types, since
 * it is the alignment the two places share.
 */
PrimitiveType TypeOfRoom(int32 room)
{
    for (PrimitiveType type : {PrimitiveType::kChar, PrimitiveType::kShort,
                               PrimitiveType::kInt, PrimitiveType::kLong})
    {
        if (PrimitiveSize(type) == room)
            return type;
    }

    std::cerr << "Error: no type takes this much room!" << std::endl;
    std::exit(1);
}

}   /* namespace */

AsmCodegen::AsmCodegen(const std::string& output_file)
    : unit_name_(UnitNameOf(output_file)),
      next_label_(1),
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

/* The file name without its directory and its suffix. */
std::string AsmCodegen::UnitNameOf(const std::string& path)
{
    std::string name = path;
    const std::size_t slash = name.find_last_of('/');

    if (slash != std::string::npos)
        name = name.substr(slash + 1);

    const std::size_t dot = name.find_last_of('.');

    if (dot != std::string::npos)
        name = name.substr(0, dot);

    return name;
}

const std::string& AsmCodegen::UnitName() const
{
    return unit_name_;
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

    /* Whatever the target gathered while the code was written, which
     * belongs after the last of it. */
    EmitEndOfProgram();
}

/*
 * Generate the code for one statement. A statement yields no value, so
 * nothing is returned. A null node is an empty compound statement.
 */
void AsmCodegen::GenStatement(const AstNodePtr& root)
{
    if (!root)
        return;

    /*
     * A whole composite value cannot be read, so the one tree of that type
     * which may be a statement is the storing of one place holding such a
     * value into another. That gives the room of the target what the other
     * place holds rather than giving a value anywhere, which is why it is
     * written where it stands rather than worked out into a register.
     */
    if (root->GetType() == PrimitiveType::kComposite)
    {
        if (root->GetAstNodeTag() == A_AstOperator &&
            static_cast<const AstOperator&>(*root).GetOpType() == T_Assign)
        {
            GenCompositeCopy(root);
            return;
        }

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

        case A_AstDoWhile:
            GenDoWhile(root);
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
            /*
             * A return which is given no value only leaves the function:
             * there is nothing to put where its caller looks for an answer.
             */
            if (!root->GetLeft())
            {
                EmitJump(function_end_label_);
                return;
            }

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
        case A_AstTernary:
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
 * Generate the code for a do loop, which is a while loop with the
 * condition at the end:
 *
 * Lstart:
 *      <body>
 * Lnext:
 *      <condition>, jumping to Lend when it does not hold
 *      jmp Lstart
 * Lend:
 *
 * The body is the first thing the loop runs, so the loop always goes round
 * at least once, however the condition reads. A continue of this loop goes
 * to the condition rather than back to the body, since what it begins is
 * the next pass and the condition is what decides whether there is one.
 */
void AsmCodegen::GenDoWhile(const AstNodePtr& root)
{
    label_idx start_label = NewLabel();
    label_idx end_label = NewLabel();
    label_idx next_label = NewLabel();

    const label_idx saved_break = loop_break_label_;
    const label_idx saved_continue = loop_continue_label_;
    loop_break_label_ = end_label;
    loop_continue_label_ = next_label;

    EmitLabel(start_label);

    GenStatement(root->GetRight());
    FreeAllRegister();

    EmitLabel(next_label);

    GenCondition(root->GetLeft(), end_label);
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

        const PrimitiveType type = root->GetLeft()->GetType();

        reg_idx case_reg = LoadInt(ast_case->GetValue(), type);
        JumpIfEqual(reg, case_reg, labels[k], type);
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
 * Generate the code for the condition of a statement or of a ternary
 * expression. What the parser hands here is either a comparison, which
 * becomes a compare followed by a jump to the false label when it does not
 * hold, or a value the parser has already asked for its truth value, which
 * becomes a jump when that value is zero.
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

        CompareAndJump(ast_op->GetOpType(),
                       left_reg,
                       right_reg,
                       false_label,
                       condition->GetLeft()->GetType());
        return;
    }

    reg_idx reg = GenExpr(condition);

    JumpIfZero(reg, false_label);
    FreeRegister(reg);
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
        BeginGlobal(symbol, SymbolAlignment(symbol));
        EmitGlobalSpace(SymbolStorageSize(symbol));
        EndGlobal();
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

    BeginGlobal(symbol, SymbolAlignment(symbol));

    for (int32 k = 0; k < element_count; k++)
    {
        /* An element given a string holds the address of that string. */
        if (string_labels[k] != 0)
        {
            EmitGlobalLabelRef(string_labels[k]);
            continue;
        }

        /*
         * An element which was not given a value starts out zero, which is
         * what an array with fewer values than elements needs.
         */
        int32 value = 0;

        if (k < static_cast<int32>(symbol.initial_values.size()))
            value = std::get<int32>(symbol.initial_values[k]);

        /*
         * A value may be wider than the element holding it, as a cast in
         * front of it says, and then the element keeps the low bits of it,
         * the way a store does.
         */
        if (element_size == 1)
            EmitGlobalValue(value & 0xff, 1);
        else
            EmitGlobalValue(value, element_size);
    }

    EndGlobal();
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
            return LoadInt(int_lit->GetValue(), root->GetType());
        }
        case A_AstCast:
        {
            reg_idx reg = GenExpr(root->GetLeft());

            return Cast(reg, root->GetLeft()->GetType(), root->GetType());
        }
        case A_AstTernary:
            return GenTernary(root);
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

            return Scale(reg, scale->GetSize(), root->GetType());
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
            return Negate(GenExpr(root->GetLeft()),
                          root->GetLeft()->GetType());
        case A_AstInvert:
            return Invert(GenExpr(root->GetLeft()),
                          root->GetLeft()->GetType());
        case A_AstLogNot:
            return LogNot(GenExpr(root->GetLeft()),
                          root->GetLeft()->GetType());
        case A_AstToBool:
            return ToBool(GenExpr(root->GetLeft()),
                          root->GetLeft()->GetType());
        case A_AstIncDec:
        {
            const AstIncDec *inc =
                static_cast<const AstIncDec*>(root.get());

            /*
             * A name is reached directly by the machine, which has a way of
             * its own of changing what one holds and is asked for it here;
             * every other place is reached through an address the code
             * generator works out, which the target is not told about.
             */
            if (inc->GetPlace()->GetAstNodeTag() == A_AstIdentifier)
            {
                const Symbol& symbol = static_cast<const AstIdentifier&>(
                    *inc->GetPlace()).GetSymbol();

                return IncDec(symbol, inc->GetDelta(), inc->IsPost());
            }

            return IncDecPlace(*inc);
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

            /* '&&' and '||' decide which of their two sides is worked out
             * at all, so both are not worked out before they are
             * combined. */
            if (op_type == T_LogAnd || op_type == T_LogOr)
                return GenLogic(root, op_type == T_LogAnd);

            /*
             * The two values are of one type, which the parser has put
             * them both into; a comparison answers with an int whatever
             * the two of them are, so it is the type they are compared as
             * which is the width of the operation.
             */
            const PrimitiveType type = IsComparisonOp(op_type)
                ? root->GetLeft()->GetType() : root->GetType();

            reg_idx left_reg = GenExpr(root->GetLeft());
            reg_idx right_reg = GenExpr(root->GetRight());

            return GenOperator(op_type, left_reg, right_reg, type);
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

        PushArgument(arg_reg, (*it)->GetType());
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

/*
 * Store the contents of one place holding a composite value into another.
 *
 * The two places hold the same type, so the same room is given over, and
 * the room is moved in as few whole moves as it takes: a piece as wide as
 * the alignment the two places share, since a value is moved whole and the
 * two places are lined up for a value of that width. The room of a
 * composite type is a whole number of pieces, since it is rounded up to
 * its alignment when the type is built.
 *
 * Both places are reached before either is read or written. Reaching a
 * place cannot change another one, since the value a pointer holds is only
 * changed by an increment, and an increment needs a name to go back into
 * rather than a place.
 */
void AsmCodegen::GenCompositeCopy(const AstNodePtr& root)
{
    const std::shared_ptr<CompositeType>& composite = root->GetComposite();
    const int32 room = ValueSize(PrimitiveType::kComposite, composite);
    const int32 piece = ValueAlignment(PrimitiveType::kComposite, composite);

    reg_idx to = GenAddress(root->GetRight());
    reg_idx from = GenAddress(root->GetLeft());

    CopyValue(to, from, room, piece);

    FreeRegister(from);
    FreeRegister(to);
}

/*
 * Move the given room from one place to another, a piece at a time. The
 * places are walked along rather than moved back to the start for every
 * piece, so a move of each piece is an offset of the piece which came
 * before it.
 */
void AsmCodegen::CopyValue(reg_idx to, reg_idx from, int32 room, int32 piece)
{
    const PrimitiveType type = TypeOfRoom(piece);

    for (int32 moved = 0; moved < room; moved = moved + piece)
    {
        if (moved != 0)
        {
            AddOffset(from, piece);
            AddOffset(to, piece);
        }

        reg_idx value = LoadDeref(from, type);

        StoreDeref(value, to, type);
        FreeRegister(value);
    }
}

// Compute an lvalue's address without loading the value stored there.
/*
 * Generate the code for '&&' or '||':
 *
 *      <the left side>, jumping to Lsettled when it settles the answer
 *      <the right side>, jumping to Lsettled when it settles it
 *      the answer which neither side settled
 *      jmp Lend
 * Lsettled:
 *      the answer the side which jumped settled
 * Lend:
 *
 * Which answer a side settles is the one which stops the question being
 * asked any further: a side which is zero settles a '&&' as zero, and one
 * which is not zero settles a '||' as one. The side which is not worked out
 * is the whole point of the operation, since it may be the side which would
 * go wrong: `if (p && *p == 1)` must not read through a pointer which points
 * at nothing.
 *
 * Each side is thrown away once it has been asked, so only one of them ever
 * holds a register; the answer is written in one register whichever way it
 * is reached, and that register is taken before either side is worked out.
 */
reg_idx AsmCodegen::GenLogic(const AstNodePtr& root, bool is_and)
{
    const label_idx settled = NewLabel();
    const label_idx end_label = NewLabel();

    const reg_idx result = AllocRegister();

    reg_idx value = GenExpr(root->GetLeft());

    if (is_and)
        JumpIfZero(value, settled);
    else
        JumpIfNonZero(value, settled);

    FreeRegister(value);

    value = GenExpr(root->GetRight());

    if (is_and)
        JumpIfZero(value, settled);
    else
        JumpIfNonZero(value, settled);

    FreeRegister(value);

    /* Neither side settled it, so the answer is the one which the other
     * side would have settled it as. */
    value = LoadInt(is_and ? 1 : 0, PrimitiveType::kInt);
    Move(result, value, PrimitiveType::kInt);
    FreeRegister(value);

    EmitJump(end_label);
    EmitLabel(settled);

    value = LoadInt(is_and ? 0 : 1, PrimitiveType::kInt);
    Move(result, value, PrimitiveType::kInt);
    FreeRegister(value);

    EmitLabel(end_label);

    return result;
}

/*
 * Generate the code for a ternary expression, of which only one of the two
 * values is ever worked out:
 *
 *      <condition>, jumping to Lfalse when it does not hold
 *      <the value taken when it holds>, into the register taken for it
 *      jmp Lend
 * Lfalse:
 *      <the value taken when it does not hold>, into that register
 * Lend:
 *
 * Whichever value is taken, the expression answers with the same register,
 * so one is taken before either side is worked out; the side which is
 * worked out may use every other register and its value is moved into the
 * one taken, which the caller is given.
 */
reg_idx AsmCodegen::GenTernary(const AstNodePtr& root)
{
    const label_idx false_label = NewLabel();
    const label_idx end_label = NewLabel();

    /*
     * The condition is a value like any other: it is worked out to say
     * which of the two values is taken and is then thrown away.
     */
    GenCondition(root->GetLeft(), false_label);

    /*
     * The register is taken before either value is worked out, so that the
     * value worked out can use every other one and is then moved into it.
     * Nothing else is thrown away here: what the expression this stands in
     * is holding is not the ternary's to free.
     */
    const reg_idx result = AllocRegister();

    reg_idx value = GenExpr(root->GetMid());
    Move(result, value, root->GetMid()->GetType());
    FreeRegister(value);

    EmitJump(end_label);
    EmitLabel(false_label);

    value = GenExpr(root->GetRight());
    Move(result, value, root->GetRight()->GetType());
    FreeRegister(value);

    EmitLabel(end_label);

    return result;
}

/*
 * Change what a place other than a name holds, giving the expression the
 * value the place held before the change or the one it holds after it.
 *
 * The place is reached through an address the code generator works out, and
 * the value there is read, changed and stored back through it. The old
 * value is kept beside the new one, since a value written after the place
 * is the one the place held before the change; a value written before it is
 * the new one, which the register holding it already has.
 */
reg_idx AsmCodegen::IncDecPlace(const AstIncDec& inc)
{
    const AstNodePtr& place = inc.GetPlace();
    const PrimitiveType type = place->GetType();
    const int32 amount = inc.GetDelta() > 0 ? inc.GetDelta() : -inc.GetDelta();

    reg_idx address = GenAddress(place);
    reg_idx value = LoadDeref(address, type);

    reg_idx before = AllocRegister();

    Move(before, value, type);

    reg_idx step = LoadInt(amount, type);

    if (inc.GetDelta() > 0)
        value = Add(value, step, type);
    else
        value = Sub(value, step, type);

    StoreDeref(value, address, type);
    FreeRegister(address);

    if (inc.IsPost())
    {
        FreeRegister(value);
        return before;
    }

    FreeRegister(before);

    return value;
}

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
            reg_idx result_reg = GenOperator(op_type, current_reg, value_reg,
                                             target->GetType());

            StoreSymbol(ident->GetSymbol(), result_reg);
            return result_reg;
        }

        case A_AstDeref:
        case A_AstMember:
        {
            reg_idx address_reg = GenAddress(target);
            reg_idx current_reg = LoadDeref(address_reg, target->GetType());
            reg_idx result_reg = GenOperator(op_type, current_reg, value_reg,
                                             target->GetType());

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
    reg_idx right_reg,
    PrimitiveType type)
{
    switch (op_type)
    {
        case T_Plus:
            return Add(left_reg, right_reg, type);
        case T_Minus:
            return Sub(left_reg, right_reg, type);
        case T_Star:
            return Mul(left_reg, right_reg, type);
        case T_Slash:
            return Div(left_reg, right_reg, type);
        case T_Percent:
            return Mod(left_reg, right_reg, type);
        case T_Amper:
            return And(left_reg, right_reg, type);
        case T_Or:
            return Or(left_reg, right_reg, type);
        case T_Xor:
            return Xor(left_reg, right_reg, type);
        case T_LShift:
            return ShiftLeft(left_reg, right_reg, type);
        case T_RShift:
            return ShiftRight(left_reg, right_reg, type);
        case T_EQ:
        case T_NE:
        case T_LT:
        case T_GT:
        case T_LE:
        case T_GE:
            return CompareAndSet(op_type, left_reg, right_reg, type);
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

    /*
     * Every register holds a value, so one of their values is set aside and
     * the register is handed out to hold a value of its own. The value set
     * aside comes back as soon as that register is free again, since the
     * code which asked for the register holds its name throughout.
     *
     * Which register this happens to is decided in turn, so that a long
     * expression sets aside the values of each of them about as often as
     * the others; a register with as many values set aside as there is room
     * for is passed over, and an expression which needs more room than the
     * frame has is refused.
     */
    for (idx_t k = 0; k < kRegSize; k++)
    {
        const reg_idx candidate = (spill_next_ + k) % kRegSize;

        if (spill_count_[candidate] == kSpillDepthPerRegister)
            continue;

        const int32 slot = candidate * kSpillDepthPerRegister +
                           spill_count_[candidate];

        SpillRegister(candidate, slot);
        spill_count_[candidate]++;
        spill_next_ = (candidate + 1) % kRegSize;

        return candidate;
    }

    std::cerr << "Error: an expression is too complex to be worked out!";
    std::cerr << std::endl;
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

    /*
     * The value the register holds is done with. If a value of this
     * register's own was set aside to make room for it, that value comes
     * back now and the register goes on holding it: the code which is
     * freeing the register has been holding its name for that value all
     * along, and will use it once the values worked out in between are
     * done with.
     */
    if (spill_count_[reg] > 0)
    {
        spill_count_[reg]--;
        ReloadRegister(reg, reg * kSpillDepthPerRegister + spill_count_[reg]);
        return;
    }

    is_free_[reg] = true;
}

void AsmCodegen::FreeAllRegister()
{
    for (idx_t i = 0; i < kRegSize; i++)
    {
        is_free_[i] = true;
        /* Whatever was set aside at the end of a statement is a value the
         * statement is done with, so it is not brought back. */
        spill_count_[i] = 0;
    }

    spill_next_ = 0;
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
