#include "nuocc_asm_codegen.hpp"

#include <stdlib.h>

#include <iostream>

namespace nuocc
{

AsmCodegen::AsmCodegen(const std::string& output_file)
    : ofs_(output_file, std::ios::out | std::ios::trunc)
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
    }
}

AsmCodegen::~AsmCodegen()
{
    ofs_.close();
}

void AsmCodegen::GenerateAsmCode(const std::unique_ptr<AstNode>& root)
{
    Preamble();
    reg_idx reg = GenAst(root);
    PrintInt(reg);
    Postamble();
}

void AsmCodegen::Preamble()
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

void AsmCodegen::Postamble()
{
    ofs_ << "\tmovl $0, %eax" << std::endl;
    ofs_ << "\tpopq %rbp" << std::endl;
    ofs_ << "\tret" << std::endl;
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

void AsmCodegen::PrintInt(reg_idx reg)
{
    ofs_ << "\tmovq\t" << reg_list_[reg] << ", %rdi" << std::endl;
    ofs_ << "\tcall\tprintint\n" << std::endl;

    FreeRegister(reg);
}

reg_idx AsmCodegen::GenAst(const std::unique_ptr<AstNode>& root)
{
    reg_idx left_reg = kRegSize, right_reg = kRegSize;

    if (root->GetLeft())
        left_reg = GenAst(root->GetLeft());
    if (root->GetRight())
        right_reg = GenAst(root->GetRight());

    switch (root->GetAstNodeTag())
    {
        case A_AstIntLit:
        {
            const AstIntLit *int_lit =
                static_cast<const AstIntLit*>(root.get());
            return Load(int_lit->GetValue());
        }
        case A_AstOperator:
            return GenAstOp(root, left_reg, right_reg);
        default:
            break;
    }

    std::cerr << "Unknown Ast Node " << root->GetAstNodeTag() << "!" << std::endl;
    std::exit(1);
}

reg_idx AsmCodegen::GenAstOp(const std::unique_ptr<AstNode>& root,
                             reg_idx left_reg,
                             reg_idx right_reg)
{
    const AstOperator *root_op =
        static_cast<const AstOperator*>(root.get());
    
    switch (root_op->GetOpType())
    {
        case T_Plus:
            return Add(left_reg, right_reg);
        case T_Minus:
            return Sub(left_reg, right_reg);
        case T_Star:
            return Mul(left_reg, right_reg);
        case T_Slash:
            return Div(left_reg, right_reg);
        default:
            break;
    }

    std::cerr << "Unknown Operator Type " << root_op->GetOpType() << "!" << std::endl;
    std::exit(1);
}

}   /* namespace nuocc */