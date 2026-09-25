#include "nuocc_asm_codegen.hpp"

#include <stdlib.h>

#include <iostream>

namespace nuocc
{

AsmCodegen::AsmCodegen(const std::string& output_file)
    : next_label_(1),
      function_end_label_(0),
      function_return_type_(PrimitiveType::kNone),
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

void AsmCodegen::GenProgram(const std::vector<AstNodePtr>& functions)
{
    FreeAllRegister();

    EmitPreamble();

    for (const AstNodePtr& function : functions)
    {
        const AstFunction *ast_function =
            static_cast<const AstFunction*>(function.get());

        /*
         * Every return statement jumps here, and the type tells the target
         * where the result of a return has to end up.
         */
        function_end_label_ = NewLabel();
        function_return_type_ = ast_function->GetSymbol().type;

        EmitFunctionPreamble(ast_function->GetSymbol());

        GenStatement(function->GetLeft());

        FreeAllRegister();

        EmitFunctionPostamble();

        function_end_label_ = 0;
        function_return_type_ = PrimitiveType::kNone;
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
        {
            const AstDeclare *decl =
                static_cast<const AstDeclare*>(root.get());
            GenGlobSymbol(decl->GetSymbol());
            return;
        }

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

        case A_AstFuncCall:
        {
            /* The result of a call is discarded when it stands alone. */
            reg_idx reg = GenCall(root);
            FreeRegister(reg);
            return;
        }

        case A_AstOperator:
        {
            const AstOperator *ast_op =
                static_cast<const AstOperator*>(root.get());

            if (ast_op->GetOpType() != T_Assign)
                break;

            /* The left child is the value to store, the right one the target. */
            const AstNodePtr& target = root->GetRight();

            if (!target || target->GetAstNodeTag() != A_AstIdentifier)
            {
                std::cerr << "Error: assignment target is not an identifier!";
                std::cerr << std::endl;
                std::exit(1);
            }

            const AstIdentifier *ident =
                static_cast<const AstIdentifier*>(target.get());

            if (!ident->GetLvIdent())
            {
                std::cerr << "Error: assignment target ";
                std::cerr << ident->GetSymbol().name;
                std::cerr << " is not an lvalue!" << std::endl;
                std::exit(1);
            }

            reg_idx value_reg = GenExpr(root->GetLeft());

            StoreGlobSymbol(ident->GetSymbol(), value_reg);
            FreeRegister(value_reg);
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
    const AstOperator *ast_op =
        static_cast<const AstOperator*>(condition.get());

    reg_idx left_reg = GenExpr(condition->GetLeft());
    reg_idx right_reg = GenExpr(condition->GetRight());

    CompareAndJump(ast_op->GetOpType(), left_reg, right_reg, false_label);
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
        case A_AstIdentifier:
        {
            const AstIdentifier *ident =
                static_cast<const AstIdentifier*>(root.get());

            /* An lvalue only names a location, it holds no value. */
            if (ident->GetLvIdent())
            {
                std::cerr << "Error: lvalue identifier ";
                std::cerr << ident->GetSymbol().name;
                std::cerr << " is used as a value!" << std::endl;
                std::exit(1);
            }

            return LoadGlobSymbol(ident->GetSymbol());
        }
        case A_AstWiden:
        {
            reg_idx reg = GenExpr(root->GetLeft());

            return Widen(reg, root->GetLeft()->GetType(), root->GetType());
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
        case A_AstFuncCall:
            return GenCall(root);
        case A_AstOperator:
        {
            const AstOperator *ast_op =
                static_cast<const AstOperator*>(root.get());
            NodeTag op_type = ast_op->GetOpType();

            if (op_type == T_Assign)
            {
                std::cerr << "Error: an assignment is not an expression!";
                std::cerr << std::endl;
                std::exit(1);
            }

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
 * Call a function, passing the value held in one register as its single
 * argument. The target decides where the result comes back from.
 */
reg_idx AsmCodegen::GenCall(const AstNodePtr& root)
{
    const AstFuncCall *call = static_cast<const AstFuncCall*>(root.get());

    reg_idx arg_reg = GenExpr(root->GetLeft());
    reg_idx out_reg = Call(call->GetSymbol(), arg_reg);

    FreeRegister(arg_reg);

    return out_reg;
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
