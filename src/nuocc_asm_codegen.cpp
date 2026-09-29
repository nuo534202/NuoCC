#include "nuocc_asm_codegen.hpp"

#include <stdlib.h>

#include <iostream>

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
                             ast_function->GetParameter());

        GenStatement(function->GetLeft());

        FreeAllRegister();

        EmitFunctionPostamble();

        function_end_label_ = 0;
        function_return_type_ = PrimitiveType::kNone;
        function_local_size_ = 0;
    }
}

/*
 * Generate the code for one statement. A statement yields no value, so
 * nothing is returned. A null node is an empty compound statement.
 */
void AsmCodegen::GenStatement(const AstNodePtr& root)
{
    if (!root)
        return;

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
 */
void AsmCodegen::GenWhile(const AstNodePtr& root)
{
    label_idx start_label = NewLabel();
    label_idx end_label = NewLabel();

    EmitLabel(start_label);

    GenCondition(root->GetLeft(), end_label);
    FreeAllRegister();

    GenStatement(root->GetRight());
    FreeAllRegister();

    EmitJump(start_label);
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
    ofs_ << "\t.data" << std::endl;
    ofs_ << "\t.globl\t" << GlobName(symbol.name) << std::endl;

    if (symbol.stype == StructuralType::kArray)
    {
        /*
         * An array is one block of room holding every element, all of
         * them starting out zero. The block is aligned to one element so
         * that each of them lands on an address of its own size, and the
         * label has to come after that alignment or it would point at the
         * padding in front of the array.
         */
        ofs_ << "\t.balign\t" << PrimitiveSize(symbol.type) << std::endl;
        ofs_ << GlobName(symbol.name) << ":" << std::endl;
        ofs_ << "\t.space\t" << SymbolStorageSize(symbol) << std::endl;
        ofs_ << "\t.text" << std::endl;
        return;
    }

    ofs_ << GlobName(symbol.name) << ":\t";

    /* How much room the variable needs is decided by its type. */
    switch (PrimitiveSize(symbol.type))
    {
        case 1:
            ofs_ << ".byte";
            break;
        case 4:
            ofs_ << ".long";
            break;
        case 8:
            ofs_ << ".quad";
            break;
        default:
            std::cerr << "Error: bad type for variable ";
            std::cerr << symbol.name << "!" << std::endl;
            std::exit(1);
    }

    ofs_ << "\t0" << std::endl;
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
    switch (root->GetAstNodeTag())
    {
        case A_AstIntLit:
        {
            const AstIntLit *int_lit =
                static_cast<const AstIntLit*>(root.get());
            return LoadInt(int_lit->GetValue());
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
 * Call a function with its optional argument. The target decides where the
 * result comes back from.
 */
reg_idx AsmCodegen::GenCall(const AstNodePtr& root)
{
    const AstFuncCall *call = static_cast<const AstFuncCall*>(root.get());

    std::optional<reg_idx> arg_reg;
    if (root->GetLeft())
        arg_reg = GenExpr(root->GetLeft());

    reg_idx out_reg = Call(call->GetSymbol(), arg_reg);

    if (arg_reg)
        FreeRegister(*arg_reg);

    return out_reg;
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
        {
            /* The child of the dereference is the pointer, not the value. */
            reg_idx address_reg = GenExpr(target->GetLeft());

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
