#include "codegen/nuocc_codegen_arm64.hpp"

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

/* The AArch64 instructions which implement a comparison operator. `cmp a, b`
 * computes a - b, so `condition` holds when the relation a OP b is true and
 * `inverse` holds when it is false. */
struct ComparisonInstr
{
    /* Names the condition for cset, used inside an expression. */
    std::string_view condition;
    /* Names the condition for the jump taken when the comparison fails. */
    std::string_view inverse;
};

const std::unordered_map<NodeTag, ComparisonInstr> kComparisonInstr = {
    {T_EQ, {"eq", "ne"}},
    {T_NE, {"ne", "eq"}},
    {T_LT, {"lt", "ge"}},
    {T_GT, {"gt", "le"}},
    {T_LE, {"le", "gt"}},
    {T_GE, {"ge", "lt"}}
};

/* The scratch register used to hold the address of a global variable. It is
 * one of the two the ABI reserves for exactly this kind of use. */
constexpr char kScratchReg[] = "x17";

/* The stack frame of a function: the frame pointer and the return address,
 * then the four registers this target allocates. */
constexpr int kFrameSize = 48;
constexpr int kSavedFrameOffset = 0;
constexpr int kSavedRegs1Offset = 16;
constexpr int kSavedRegs2Offset = 32;

}   /* namespace */

Arm64Codegen::Arm64Codegen(const std::string& output_file)
    : AsmCodegen(output_file)
{
    /* The last four of the callee saved registers, so that a call leaves
     * every value an expression is still holding untouched. */
    for (idx_t i = 0; i < kRegSize; i++)
    {
        xreg_list_[i] = "x" + std::to_string(i + 19);
        wreg_list_[i] = "w" + std::to_string(i + 19);
    }
}

void Arm64Codegen::EmitPreamble()
{
    ofs_ << "\t.text" << std::endl;
}

void Arm64Codegen::EmitFunctionPreamble(const Symbol& symbol)
{
    ofs_ << "\t.text" << std::endl;
    ofs_ << "\t.globl\t" << symbol.name << std::endl;
    ofs_ << "\t.type\t" << symbol.name << ", %function" << std::endl;
    ofs_ << symbol.name << ":" << std::endl;

    /* The stack has to stay a multiple of sixteen bytes at every call. */
    ofs_ << "\tstp\tx29, x30, [sp, #-" << kFrameSize << "]!" << std::endl;
    ofs_ << "\tmov\tx29, sp" << std::endl;
    ofs_ << "\tstp\t" << xreg_list_[0] << ", " << xreg_list_[1];
    ofs_ << ", [sp, #" << kSavedRegs1Offset << "]" << std::endl;
    ofs_ << "\tstp\t" << xreg_list_[2] << ", " << xreg_list_[3];
    ofs_ << ", [sp, #" << kSavedRegs2Offset << "]" << std::endl;
}

void Arm64Codegen::EmitFunctionPostamble()
{
    EmitLabel(function_end_label_);

    ofs_ << "\tldp\t" << xreg_list_[0] << ", " << xreg_list_[1];
    ofs_ << ", [sp, #" << kSavedRegs1Offset << "]" << std::endl;
    ofs_ << "\tldp\t" << xreg_list_[2] << ", " << xreg_list_[3];
    ofs_ << ", [sp, #" << kSavedRegs2Offset << "]" << std::endl;
    ofs_ << "\tldp\tx29, x30, [sp], #" << kFrameSize << std::endl;
    ofs_ << "\tret" << std::endl;
}

void Arm64Codegen::EmitLabel(label_idx label)
{
    ofs_ << "L" << label << ":" << std::endl;
}

void Arm64Codegen::EmitJump(label_idx label)
{
    ofs_ << "\tb\tL" << label << std::endl;
}

/*
 * Load a register with the address of a global variable. The address is
 * built from the page and the page offset of the symbol, which is how a
 * position independent reference is written.
 */
void Arm64Codegen::EmitGlobAddress(const Symbol& symbol)
{
    ofs_ << "\tadrp\t" << kScratchReg << ", " << symbol.name << "@PAGE";
    ofs_ << std::endl;
    ofs_ << "\tadd\t" << kScratchReg << ", " << kScratchReg << ", ";
    ofs_ << symbol.name << "@PAGEOFF" << std::endl;
}

/*
 * Put a constant into a register. An AArch64 instruction can only hold
 * sixteen bits of it at a time, so the value is written one part at a time;
 * the parts which are zero and come after a part which is not are left out.
 */
void Arm64Codegen::EmitLoadImmediate(reg_idx reg, int64 value)
{
    uint64_t bits = static_cast<uint64_t>(value);
    bool started = false;

    for (int shift = 0; shift < 64; shift += 16)
    {
        uint32_t part = static_cast<uint32_t>((bits >> shift) & 0xFFFF);

        if (part == 0 && started)
            continue;

        ofs_ << "\t" << (started ? "movk" : "movz") << "\t";
        ofs_ << xreg_list_[reg] << ", #" << part;

        if (shift != 0)
            ofs_ << ", lsl #" << shift;

        ofs_ << std::endl;
        started = true;
    }
}

void Arm64Codegen::GenGlobSymbol(const Symbol& symbol)
{
    /* The storage a variable needs is decided by its type. */
    int32 size = PrimitiveSize(symbol.type);

    ofs_ << "\t.comm\t" << symbol.name << "," << size << "," << size;
    ofs_ << std::endl;
}

reg_idx Arm64Codegen::LoadInt(int32 value)
{
    reg_idx reg = AllocRegister();

    EmitLoadImmediate(reg, value);

    return reg;
}

reg_idx Arm64Codegen::LoadGlobSymbol(const Symbol& symbol)
{
    reg_idx idx = AllocRegister();

    EmitGlobAddress(symbol);

    /*
     * Each of these leaves the whole register holding the value: loading a
     * byte or four bytes into a w register clears the rest of the x
     * register it names.
     */
    switch (symbol.type)
    {
        case PrimitiveType::kChar:
            ofs_ << "\tldrb\t" << wreg_list_[idx] << ", [" << kScratchReg;
            ofs_ << "]" << std::endl;
            break;
        case PrimitiveType::kInt:
            ofs_ << "\tldr\t" << wreg_list_[idx] << ", [" << kScratchReg;
            ofs_ << "]" << std::endl;
            break;
        case PrimitiveType::kLong:
            ofs_ << "\tldr\t" << xreg_list_[idx] << ", [" << kScratchReg;
            ofs_ << "]" << std::endl;
            break;
        default:
            std::cerr << "Error: bad type for variable ";
            std::cerr << symbol.name << "!" << std::endl;
            std::exit(1);
    }

    return idx;
}

reg_idx Arm64Codegen::StoreGlobSymbol(const Symbol& symbol, reg_idx reg)
{
    EmitGlobAddress(symbol);

    switch (symbol.type)
    {
        case PrimitiveType::kChar:
            ofs_ << "\tstrb\t" << wreg_list_[reg] << ", [";
            break;
        case PrimitiveType::kInt:
            ofs_ << "\tstr\t" << wreg_list_[reg] << ", [";
            break;
        case PrimitiveType::kLong:
            ofs_ << "\tstr\t" << xreg_list_[reg] << ", [";
            break;
        default:
            std::cerr << "Error: bad type for variable ";
            std::cerr << symbol.name << "!" << std::endl;
            std::exit(1);
    }

    ofs_ << kScratchReg << "]" << std::endl;

    return reg;
}

reg_idx Arm64Codegen::Add(reg_idx reg1, reg_idx reg2)
{
    ofs_ << "\tadd\t" << xreg_list_[reg2] << ", " << xreg_list_[reg2];
    ofs_ << ", " << xreg_list_[reg1] << std::endl;

    FreeRegister(reg1);

    return reg2;
}

reg_idx Arm64Codegen::Sub(reg_idx reg1, reg_idx reg2)
{
    ofs_ << "\tsub\t" << xreg_list_[reg1] << ", " << xreg_list_[reg1];
    ofs_ << ", " << xreg_list_[reg2] << std::endl;

    FreeRegister(reg2);

    return reg1;
}

reg_idx Arm64Codegen::Mul(reg_idx reg1, reg_idx reg2)
{
    ofs_ << "\tmul\t" << xreg_list_[reg2] << ", " << xreg_list_[reg2];
    ofs_ << ", " << xreg_list_[reg1] << std::endl;

    FreeRegister(reg1);

    return reg2;
}

reg_idx Arm64Codegen::Div(reg_idx reg1, reg_idx reg2)
{
    ofs_ << "\tsdiv\t" << xreg_list_[reg1] << ", " << xreg_list_[reg1];
    ofs_ << ", " << xreg_list_[reg2] << std::endl;

    FreeRegister(reg2);

    return reg1;
}

reg_idx Arm64Codegen::Widen(reg_idx reg,
    PrimitiveType /*old_type*/,
    PrimitiveType /*new_type*/)
{
    /*
     * Nothing to do on AArch64. A narrow value is loaded with a widening
     * load, which already zeroes the whole register, and a value computed
     * from it is just as clean; storing it back truncates again.
     */
    return reg;
}

reg_idx Arm64Codegen::CompareAndSet(NodeTag op_type, reg_idx reg1, reg_idx reg2)
{
    auto it = kComparisonInstr.find(op_type);

    if (it == kComparisonInstr.end())
    {
        std::cerr << "Error: token " << op_type;
        std::cerr << " is not a comparison!" << std::endl;
        std::exit(1);
    }

    /* cmp computes reg1 - reg2, and cset leaves a clean 0 or 1 behind. */
    ofs_ << "\tcmp\t" << xreg_list_[reg1] << ", " << xreg_list_[reg2];
    ofs_ << std::endl;
    ofs_ << "\tcset\t" << xreg_list_[reg2] << ", ";
    ofs_ << it->second.condition << std::endl;

    FreeRegister(reg1);

    return reg2;
}

void Arm64Codegen::CompareAndJump(NodeTag op_type,
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
    ofs_ << "\tcmp\t" << xreg_list_[reg1] << ", " << xreg_list_[reg2];
    ofs_ << std::endl;
    ofs_ << "\tb." << it->second.inverse << "\tL" << label << std::endl;

    FreeAllRegister();
}

/*
 * Call a function. The value registers are callee saved, so whatever an
 * expression is still holding survives the call on its own.
 */
reg_idx Arm64Codegen::Call(const Symbol& symbol, reg_idx arg_reg)
{
    reg_idx out_reg = AllocRegister();

    ofs_ << "\tmov\tx0, " << xreg_list_[arg_reg] << std::endl;
    ofs_ << "\tbl\t" << symbol.name << std::endl;
    ofs_ << "\tmov\t" << xreg_list_[out_reg] << ", x0" << std::endl;

    return out_reg;
}

/*
 * Return from the function being generated. The result goes in x0, where
 * the caller expects to find it, and control jumps to the end label.
 */
void Arm64Codegen::Return(reg_idx reg)
{
    switch (function_return_type_)
    {
        case PrimitiveType::kChar:
            ofs_ << "\tuxtb\tw0, " << wreg_list_[reg] << std::endl;
            break;
        case PrimitiveType::kInt:
            ofs_ << "\tmov\tw0, " << wreg_list_[reg] << std::endl;
            break;
        case PrimitiveType::kLong:
            ofs_ << "\tmov\tx0, " << xreg_list_[reg] << std::endl;
            break;
        default:
            std::cerr << "Error: bad return type in a return statement!";
            std::cerr << std::endl;
            std::exit(1);
    }

    EmitJump(function_end_label_);
}

void Arm64Codegen::PrintInt(reg_idx reg)
{
    ofs_ << "\tmov\tx0, " << xreg_list_[reg] << std::endl;
    ofs_ << "\tbl\t" << kPrintIntName << std::endl;
}

std::unique_ptr<AsmCodegen> MakeCodegen(const std::string& output_file)
{
    return std::make_unique<Arm64Codegen>(output_file);
}

}   /* namespace nuocc */
