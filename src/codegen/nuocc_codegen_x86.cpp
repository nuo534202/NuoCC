#include "codegen/nuocc_codegen_x86.hpp"

#include <stdlib.h>

#include <iostream>
#include <string_view>
#include <unordered_map>

#include "utils/nuocc_runtime.hpp"
#include "utils/nuocc_type_check.hpp"

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

X86Codegen::X86Codegen(const std::string& output_file)
    : AsmCodegen(output_file)
{
    for (idx_t i = 0; i < kRegSize; i++)
    {
        reg_list_[i] = "%r" + std::to_string(i + 8);
        breg_list_[i] = reg_list_[i] + "b";
        dreg_list_[i] = reg_list_[i] + "d";
    }
}

/*
 * The preamble is what every program needs regardless of the functions it
 * declares. The only thing the language can currently call is printint(),
 * which is a normal C function linked in with the generated code.
 */
void X86Codegen::EmitPreamble()
{
    ofs_ << "\t.text" << std::endl;
}

void X86Codegen::EmitFunctionPreamble(const Symbol& symbol)
{
    ofs_ << "\t.text" << std::endl;
    ofs_ << "\t.globl\t" << symbol.name << std::endl;
    ofs_ << "\t.type\t" << symbol.name << ", @function" << std::endl;
    ofs_ << symbol.name << ":" << std::endl;
    ofs_ << "\tpushq\t%rbp" << std::endl;
    ofs_ << "\tmovq\t%rsp, %rbp" << std::endl;
}

void X86Codegen::EmitFunctionPostamble()
{
    EmitLabel(function_end_label_);

    ofs_ << "\tpopq %rbp" << std::endl;
    ofs_ << "\tret" << std::endl;
}

void X86Codegen::EmitLabel(label_idx label)
{
    ofs_ << "L" << label << ":" << std::endl;
}

void X86Codegen::EmitJump(label_idx label)
{
    ofs_ << "\tjmp\tL" << label << std::endl;
}

void X86Codegen::GenGlobSymbol(const Symbol& symbol)
{
    /* The storage a variable needs is decided by its type. */
    int32 size = PrimitiveSize(symbol.type);

    ofs_ << "\t.comm\t" << symbol.name << "," << size << "," << size;
    ofs_ << std::endl;
}

reg_idx X86Codegen::LoadInt(int32 value)
{
    reg_idx reg = AllocRegister();

    ofs_ << "\tmovq\t$" << value << ", " << reg_list_[reg] << std::endl;

    return reg;
}

reg_idx X86Codegen::LoadGlobSymbol(const Symbol& symbol)
{
    reg_idx idx = AllocRegister();

    /*
     * Every one of these leaves the whole register holding the value: the
     * widening loads clear the rest of it, and writing to a 32-bit register
     * clears its upper half.
     */
    switch (symbol.type)
    {
        case PrimitiveType::kChar:
            ofs_ << "\tmovzbq\t" << symbol.name << "(%rip), ";
            ofs_ << reg_list_[idx] << std::endl;
            break;
        case PrimitiveType::kInt:
            ofs_ << "\tmovl\t" << symbol.name << "(%rip), ";
            ofs_ << dreg_list_[idx] << std::endl;
            break;
        case PrimitiveType::kLong:
            ofs_ << "\tmovq\t" << symbol.name << "(%rip), ";
            ofs_ << reg_list_[idx] << std::endl;
            break;
        default:
            std::cerr << "Error: bad type for variable ";
            std::cerr << symbol.name << "!" << std::endl;
            std::exit(1);
    }

    return idx;
}

reg_idx X86Codegen::StoreGlobSymbol(const Symbol& symbol, reg_idx reg)
{
    switch (symbol.type)
    {
        case PrimitiveType::kChar:
            ofs_ << "\tmovb\t" << breg_list_[reg] << ", ";
            break;
        case PrimitiveType::kInt:
            ofs_ << "\tmovl\t" << dreg_list_[reg] << ", ";
            break;
        case PrimitiveType::kLong:
            ofs_ << "\tmovq\t" << reg_list_[reg] << ", ";
            break;
        default:
            std::cerr << "Error: bad type for variable ";
            std::cerr << symbol.name << "!" << std::endl;
            std::exit(1);
    }

    ofs_ << symbol.name << "(%rip)" << std::endl;

    return reg;
}

reg_idx X86Codegen::Add(reg_idx reg1, reg_idx reg2)
{
    ofs_ << "\taddq\t" << reg_list_[reg1] << ", ";
    ofs_ << reg_list_[reg2] << std::endl;

    FreeRegister(reg1);

    return reg2;
}

reg_idx X86Codegen::Sub(reg_idx reg1, reg_idx reg2)
{
    ofs_ << "\tsubq\t" << reg_list_[reg2] << ", ";
    ofs_ << reg_list_[reg1] << std::endl;

    FreeRegister(reg2);

    return reg1;
}

reg_idx X86Codegen::Mul(reg_idx reg1, reg_idx reg2)
{
    ofs_ << "\timulq\t" << reg_list_[reg1] << ", ";
    ofs_ << reg_list_[reg2] << std::endl;

    FreeRegister(reg1);

    return reg2;
}

reg_idx X86Codegen::Div(reg_idx reg1, reg_idx reg2)
{
    ofs_ << "\tmovq\t" << reg_list_[reg1] << ", %rax" << std::endl;
    ofs_ << "\tcqo" << std::endl;
    ofs_ << "\tidivq\t" << reg_list_[reg2] << std::endl;
    ofs_ << "\tmovq\t%rax, " << reg_list_[reg1] << std::endl;

    FreeRegister(reg2);

    return reg1;
}

reg_idx X86Codegen::Widen(reg_idx reg,
    PrimitiveType /*old_type*/,
    PrimitiveType /*new_type*/)
{
    /*
     * Nothing to do on x86-64. A narrow value is loaded with a widening
     * load, which already zeroes the whole register, and a value computed
     * from it is just as clean; storing it back truncates again.
     */
    return reg;
}

reg_idx X86Codegen::CompareAndSet(NodeTag op_type, reg_idx reg1, reg_idx reg2)
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

void X86Codegen::CompareAndJump(NodeTag op_type,
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

/*
 * Call a function. The call is allowed to clobber every register, so the
 * ones which still hold a value have to be saved across it. An odd number
 * of saved registers would leave the stack misaligned for the call, so it
 * is padded back to a multiple of sixteen bytes.
 */
reg_idx X86Codegen::Call(const Symbol& symbol, reg_idx arg_reg)
{
    const bool *is_free = FreeRegisters();
    int32 saved = 0;

    for (reg_idx reg = 0; reg < kRegSize; reg++)
    {
        if (!is_free[reg] && reg != arg_reg)
        {
            ofs_ << "\tpushq\t" << reg_list_[reg] << std::endl;
            saved++;
        }
    }

    if (saved % 2 != 0)
        ofs_ << "\tsubq\t$8, %rsp" << std::endl;

    ofs_ << "\tmovq\t" << reg_list_[arg_reg] << ", %rdi" << std::endl;
    ofs_ << "\tcall\t" << symbol.name << std::endl;

    if (saved % 2 != 0)
        ofs_ << "\taddq\t$8, %rsp" << std::endl;

    for (reg_idx reg = kRegSize; reg > 0; reg--)
    {
        if (!is_free[reg - 1] && reg - 1 != arg_reg)
            ofs_ << "\tpopq\t" << reg_list_[reg - 1] << std::endl;
    }

    reg_idx out_reg = AllocRegister();

    ofs_ << "\tmovq\t%rax, " << reg_list_[out_reg] << std::endl;

    return out_reg;
}

/*
 * Return from the function being generated. The result goes in %rax, where
 * the caller expects to find it, and control jumps to the end label.
 */
void X86Codegen::Return(reg_idx reg)
{
    switch (function_return_type_)
    {
        case PrimitiveType::kChar:
            ofs_ << "\tmovzbl\t" << breg_list_[reg] << ", %eax" << std::endl;
            break;
        case PrimitiveType::kInt:
            ofs_ << "\tmovl\t" << dreg_list_[reg] << ", %eax" << std::endl;
            break;
        case PrimitiveType::kLong:
            ofs_ << "\tmovq\t" << reg_list_[reg] << ", %rax" << std::endl;
            break;
        default:
            std::cerr << "Error: bad return type in a return statement!";
            std::cerr << std::endl;
            std::exit(1);
    }

    EmitJump(function_end_label_);
}

void X86Codegen::PrintInt(reg_idx reg)
{
    ofs_ << "\tmovq\t" << reg_list_[reg] << ", %rdi" << std::endl;
    ofs_ << "\tcall\t" << kPrintIntName << std::endl;
    ofs_ << std::endl;
}

std::unique_ptr<AsmCodegen> MakeCodegen(const std::string& output_file)
{
    return std::make_unique<X86Codegen>(output_file);
}

}   /* namespace nuocc */
