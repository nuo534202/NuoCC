#include "nuocc_asm_codegen.hpp"

#include <stdlib.h>

#include <iostream>
#include <unordered_map>

namespace nuocc
{

namespace
{

/* The x86-64 instructions which implement a comparison operator. */
struct ComparisonInstr
{
    /* Sets a register to 0 or 1, used inside an expression. */
    std::string_view set;
    /* Jumps away when the comparison does not hold, used as a condition. */
    std::string_view jump;
};

const std::unordered_map<NodeTag, ComparisonInstr> kComparisonInstr = {
    {T_EQ, {"sete",  "jne"}},
    {T_NE, {"setne", "je"}},
    {T_LT, {"setl",  "jge"}},
    {T_GT, {"setg",  "jle"}},
    {T_LE, {"setle", "jg"}},
    {T_GE, {"setge", "jl"}}
};

}   /* namespace */

AsmCodegen::AsmCodegen(const std::string& output_file)
    : next_label_(1),
      ofs_(output_file, std::ios::out | std::ios::trunc)
{
    if (!ofs_.is_open())
    {
        std::cerr << "Error: cannot open output file!" << std::endl;
        std::exit(1);
    }

    for (idx_t i = 0; i < kRegSize; i++)
    {
        is_free_[i] = true;
        reg_list_[i] = "%r" + std::to_string(i + 8);
        breg_list_[i] = "%r" + std::to_string(i + 8) + "b";
    }
}

AsmCodegen::~AsmCodegen()
{
    ofs_.close();
}

void AsmCodegen::GenProgram(const AstNodePtr& root)
{
    GenPreamble();

    GenStatement(root);

    FreeAllRegister();

    GenPostamble();
}

void AsmCodegen::GenPreamble()
{
    FreeAllRegister();

    ofs_ << "\t.text" << std::endl;
    ofs_ << ".LC0:" << std::endl;
    ofs_ << "\t.string\t\"%d\\n\"" << std::endl;

    ofs_ << "printint:" << std::endl;
    ofs_ << "\tpushq\t%rbp" << std::endl;
    ofs_ << "\tmovq\t%rsp, %rbp" << std::endl;
    ofs_ << "\tsubq\t$16, %rsp" << std::endl;
    ofs_ << "\tmovl\t%edi, -4(%rbp)" << std::endl;
    ofs_ << "\tmovl\t-4(%rbp), %eax" << std::endl;
    ofs_ << "\tmovl\t%eax, %esi" << std::endl;
    ofs_ << "\tleaq\t.LC0(%rip), %rdi" << std::endl;
    ofs_ << "\tmovl\t$0, %eax" << std::endl;
    ofs_ << "\tcall\tprintf@PLT" << std::endl;
    ofs_ << "\tnop" << std::endl;
    ofs_ << "\tleave" << std::endl;
    ofs_ << "\tret" << std::endl;

    ofs_ << std::endl;

    ofs_ << "\t.globl\tmain" << std::endl;
    ofs_ << "\t.type\tmain, @function" << std::endl;
    ofs_ << "main:" << std::endl;
    ofs_ << "\tpushq\t%rbp" << std::endl;
    ofs_ << "\tmovq\t%rsp, %rbp" << std::endl;
}

void AsmCodegen::GenPostamble()
{
    ofs_ << "\tmovl $0, %eax" << std::endl;
    ofs_ << "\tpopq %rbp" << std::endl;
    ofs_ << "\tret" << std::endl;
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
                std::cerr << "Error: assignment target " << ident->GetSymbol();
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
 * Generate the code for an if condition. The condition has to be a
 * comparison, so it becomes a compare followed by a jump to the false
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
            return Load(int_lit->GetValue());
        }
        case A_AstIdentifier:
        {
            const AstIdentifier *ident =
                static_cast<const AstIdentifier*>(root.get());

            /* An lvalue only names a location, it holds no value. */
            if (ident->GetLvIdent())
            {
                std::cerr << "Error: lvalue identifier ";
                std::cerr << ident->GetSymbol();
                std::cerr << " is used as a value!" << std::endl;
                std::exit(1);
            }

            return LoadGlobSymbol(ident->GetSymbol());
        }
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

void AsmCodegen::GenGlobSymbol(const Symbol& symbol)
{
    ofs_ << "\t.comm\t" << symbol << ",8,8" << std::endl;
}

reg_idx AsmCodegen::LoadGlobSymbol(const Symbol& symbol)
{
    reg_idx idx = AllocRegister();

    ofs_ << "\tmovq\t" << symbol << "(%rip), ";
    ofs_ << reg_list_[idx] << std::endl;

    return idx;
}

reg_idx AsmCodegen::StoreGlobSymbol(const Symbol& symbol, reg_idx reg)
{
    ofs_ << "\tmovq\t" << reg_list_[reg] << ", ";
    ofs_ << symbol << "(%rip)" << std::endl;

    return reg;
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

void AsmCodegen::EmitLabel(label_idx label)
{
    ofs_ << "L" << label << ":" << std::endl;
}

void AsmCodegen::EmitJump(label_idx label)
{
    ofs_ << "\tjmp\tL" << label << std::endl;
}

reg_idx AsmCodegen::Load(int32 value)
{
    reg_idx reg = AllocRegister();

    ofs_ << "\tmovq\t$" << value << ", " << reg_list_[reg] << std::endl;

    return reg;
}

reg_idx AsmCodegen::Add(reg_idx reg1, reg_idx reg2)
{
    ofs_ << "\taddq\t" << reg_list_[reg1] << ", ";
    ofs_ << reg_list_[reg2] << std::endl;

    FreeRegister(reg1);

    return reg2;
}

reg_idx AsmCodegen::Sub(reg_idx reg1, reg_idx reg2)
{
    ofs_ << "\tsubq\t" << reg_list_[reg2] << ", ";
    ofs_ << reg_list_[reg1] << std::endl;

    FreeRegister(reg2);

    return reg1;
}

reg_idx AsmCodegen::Mul(reg_idx reg1, reg_idx reg2)
{
    ofs_ << "\timulq\t" << reg_list_[reg1] << ", ";
    ofs_ << reg_list_[reg2] << std::endl;

    FreeRegister(reg1);

    return reg2;
}

reg_idx AsmCodegen::Div(reg_idx reg1, reg_idx reg2)
{
    ofs_ << "\tmovq\t" << reg_list_[reg1] << ", %rax" << std::endl;
    ofs_ << "\tcqo" << std::endl;
    ofs_ << "\tidivq\t" << reg_list_[reg2] << std::endl;
    ofs_ << "\tmovq\t%rax, " << reg_list_[reg1] << std::endl;

    FreeRegister(reg2);

    return reg1;
}

reg_idx AsmCodegen::CompareAndSet(NodeTag op_type, reg_idx reg1, reg_idx reg2)
{
    auto it = kComparisonInstr.find(op_type);

    if (it == kComparisonInstr.end())
    {
        std::cerr << "Error: token " << op_type;
        std::cerr << " is not a comparison!" << std::endl;
        std::exit(1);
    }

    /*
     * cmpq computes reg1 - reg2, so the setX instruction reports the
     * requested relation between the two registers. It only writes the
     * low byte of reg2, so movzbq extends it to a clean 0 or 1.
     */
    ofs_ << "\tcmpq\t" << reg_list_[reg2] << ", ";
    ofs_ << reg_list_[reg1] << std::endl;
    ofs_ << "\t" << it->second.set << "\t" << breg_list_[reg2] << std::endl;
    ofs_ << "\tmovzbq\t" << breg_list_[reg2] << ", ";
    ofs_ << reg_list_[reg2] << std::endl;

    FreeRegister(reg1);

    return reg2;
}

void AsmCodegen::CompareAndJump(NodeTag op_type,
    reg_idx reg1,
    reg_idx reg2,
    label_idx label)
{
    auto it = kComparisonInstr.find(op_type);

    if (it == kComparisonInstr.end())
    {
        std::cerr << "Error: token " << op_type;
        std::cerr << " is not a comparison!" << std::endl;
        std::exit(1);
    }

    /*
     * The jump is taken when the comparison does not hold, so the code
     * which follows only runs when the condition is true.
     */
    ofs_ << "\tcmpq\t" << reg_list_[reg2] << ", ";
    ofs_ << reg_list_[reg1] << std::endl;
    ofs_ << "\t" << it->second.jump << "\tL" << label << std::endl;

    FreeAllRegister();
}

void AsmCodegen::PrintInt(reg_idx reg)
{
    ofs_ << "\tmovq\t" << reg_list_[reg] << ", %rdi" << std::endl;
    ofs_ << "\tcall\tprintint\n" << std::endl;

    FreeRegister(reg);
}

}   /* namespace nuocc */
