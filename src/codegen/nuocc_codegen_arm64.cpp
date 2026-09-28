#include "codegen/nuocc_codegen_arm64.hpp"

#include <stdlib.h>

#include <bit>
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
constexpr int kBaseFrameSize = 48;
constexpr int kSavedRegs1Offset = 16;
constexpr int kSavedRegs2Offset = 32;

/*
 * Mach-O decorates the name of every global symbol with a leading
 * underscore, ELF leaves the name alone. This is the only difference
 * between the two as far as a name is concerned, and it is decided by the
 * system the compiler runs on, which is the one whose assembler will read
 * the output.
 */
#ifdef __APPLE__
constexpr std::string_view kGlobalPrefix = "_";
#else
constexpr std::string_view kGlobalPrefix = "";
#endif

}   /* namespace */

/*
 * The name of a global symbol as this assembler wants it written, which is
 * the name on its own everywhere but on Mach-O.
 */
std::string Arm64Codegen::GlobName(const std::string& name) const
{
    return std::string(kGlobalPrefix) + name;
}

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

void Arm64Codegen::EmitFunctionPreamble(
    const Symbol& symbol,
    int32 local_size,
    const std::optional<Symbol>& parameter)
{
    ofs_ << "\t.text" << std::endl;
    ofs_ << "\t.globl\t" << GlobName(symbol.name) << std::endl;
    ofs_ << GlobName(symbol.name) << ":" << std::endl;

    /* The stack has to stay a multiple of sixteen bytes at every call. */
    frame_size_ = ((kBaseFrameSize + local_size + 15) / 16) * 16;
    ofs_ << "\tsub\tsp, sp, #" << frame_size_ << std::endl;
    ofs_ << "\tstp\tx29, x30, [sp]" << std::endl;
    ofs_ << "\tmov\tx29, sp" << std::endl;
    ofs_ << "\tstp\t" << xreg_list_[0] << ", " << xreg_list_[1];
    ofs_ << ", [sp, #" << kSavedRegs1Offset << "]" << std::endl;
    ofs_ << "\tstp\t" << xreg_list_[2] << ", " << xreg_list_[3];
    ofs_ << ", [sp, #" << kSavedRegs2Offset << "]" << std::endl;

    if (parameter)
    {
        const int32 offset = kBaseFrameSize + parameter->stack_offset;
        switch (PrimitiveSize(parameter->type))
        {
            case 1:
                ofs_ << "\tstrb\tw0, [x29, #" << offset << "]" << std::endl;
                break;
            case 4:
                ofs_ << "\tstr\tw0, [x29, #" << offset << "]" << std::endl;
                break;
            case 8:
                ofs_ << "\tstr\tx0, [x29, #" << offset << "]" << std::endl;
                break;
            default:
                std::cerr << "Error: invalid parameter type!" << std::endl;
                std::exit(1);
        }
    }
}

void Arm64Codegen::EmitFunctionPostamble()
{
    EmitLabel(function_end_label_);

    ofs_ << "\tldp\t" << xreg_list_[0] << ", " << xreg_list_[1];
    ofs_ << ", [sp, #" << kSavedRegs1Offset << "]" << std::endl;
    ofs_ << "\tldp\t" << xreg_list_[2] << ", " << xreg_list_[3];
    ofs_ << ", [sp, #" << kSavedRegs2Offset << "]" << std::endl;
    ofs_ << "\tldp\tx29, x30, [sp]" << std::endl;
    ofs_ << "\tadd\tsp, sp, #" << frame_size_ << std::endl;
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
    ofs_ << "\tadrp\t" << kScratchReg << ", " << GlobName(symbol.name);
    ofs_ << "@PAGE" << std::endl;
    ofs_ << "\tadd\t" << kScratchReg << ", " << kScratchReg << ", ";
    ofs_ << GlobName(symbol.name) << "@PAGEOFF" << std::endl;
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

reg_idx Arm64Codegen::LoadInt(int32 value)
{
    reg_idx reg = AllocRegister();

    EmitLoadImmediate(reg, value);

    return reg;
}

reg_idx Arm64Codegen::LoadSymbol(const Symbol& symbol)
{
    reg_idx idx = AllocRegister();

    if (symbol.storage == StorageClass::kLocal)
    {
        const int32 offset = kBaseFrameSize + symbol.stack_offset;
        switch (PrimitiveSize(symbol.type))
        {
            case 1:
                ofs_ << "\tldrb\t" << wreg_list_[idx] << ", [x29, #";
                break;
            case 4:
                ofs_ << "\tldr\t" << wreg_list_[idx] << ", [x29, #";
                break;
            case 8:
                ofs_ << "\tldr\t" << xreg_list_[idx] << ", [x29, #";
                break;
            default:
                std::cerr << "Error: bad type for variable " << symbol.name;
                std::cerr << "!" << std::endl;
                std::exit(1);
        }
        ofs_ << offset << "]" << std::endl;
        return idx;
    }

    EmitGlobAddress(symbol);

    /*
     * How much is read is decided by the type's size. Loading a byte or
     * four bytes into a w register clears the rest of the x register it
     * names, so the whole register holds the value either way.
     */
    switch (PrimitiveSize(symbol.type))
    {
        case 1:
            ofs_ << "\tldrb\t" << wreg_list_[idx] << ", [" << kScratchReg;
            ofs_ << "]" << std::endl;
            break;
        case 4:
            ofs_ << "\tldr\t" << wreg_list_[idx] << ", [" << kScratchReg;
            ofs_ << "]" << std::endl;
            break;
        case 8:
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

reg_idx Arm64Codegen::StoreSymbol(const Symbol& symbol, reg_idx reg)
{
    if (symbol.storage == StorageClass::kLocal)
    {
        const int32 offset = kBaseFrameSize + symbol.stack_offset;
        switch (PrimitiveSize(symbol.type))
        {
            case 1:
                ofs_ << "\tstrb\t" << wreg_list_[reg] << ", [x29, #";
                break;
            case 4:
                ofs_ << "\tstr\t" << wreg_list_[reg] << ", [x29, #";
                break;
            case 8:
                ofs_ << "\tstr\t" << xreg_list_[reg] << ", [x29, #";
                break;
            default:
                std::cerr << "Error: bad type for variable " << symbol.name;
                std::cerr << "!" << std::endl;
                std::exit(1);
        }
        ofs_ << offset << "]" << std::endl;
        return reg;
    }

    EmitGlobAddress(symbol);

    switch (PrimitiveSize(symbol.type))
    {
        case 1:
            ofs_ << "\tstrb\t" << wreg_list_[reg] << ", [";
            break;
        case 4:
            ofs_ << "\tstr\t" << wreg_list_[reg] << ", [";
            break;
        case 8:
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

/*
 * Multiply the value in a register by the size of a type. Every size the
 * language has now is a power of two, so a shift left is enough and
 * cheaper than a multiply; the multiply is there for the sizes which are
 * not, which the composite types later on will bring.
 */
reg_idx Arm64Codegen::Scale(reg_idx reg, int32 scale)
{
    uint32 size = static_cast<uint32>(scale);

    if (std::has_single_bit(size))
    {
        ofs_ << "\tlsl\t" << xreg_list_[reg] << ", " << xreg_list_[reg];
        ofs_ << ", #" << std::countr_zero(size) << std::endl;

        return reg;
    }

    reg_idx size_reg = LoadInt(scale);

    return Mul(reg, size_reg);
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
reg_idx Arm64Codegen::Call(const Symbol& symbol,
    std::optional<reg_idx> arg_reg)
{
    reg_idx out_reg = AllocRegister();

    if (arg_reg)
        ofs_ << "\tmov\tx0, " << xreg_list_[*arg_reg] << std::endl;
    ofs_ << "\tbl\t" << GlobName(symbol.name) << std::endl;
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
    ofs_ << "\tbl\t" << GlobName(kPrintIntName) << std::endl;
}

/*
 * Load the address of a global variable into a fresh register.
 */
reg_idx Arm64Codegen::AddressOf(const Symbol& symbol)
{
    reg_idx reg = AllocRegister();

    if (symbol.storage == StorageClass::kLocal)
    {
        const int32 offset = kBaseFrameSize + symbol.stack_offset;
        ofs_ << "\tadd\t" << xreg_list_[reg] << ", x29, #";
        ofs_ << offset << std::endl;
        return reg;
    }

    ofs_ << "\tadrp\t" << xreg_list_[reg] << ", " << GlobName(symbol.name);
    ofs_ << "@PAGE" << std::endl;
    ofs_ << "\tadd\t" << xreg_list_[reg] << ", " << xreg_list_[reg] << ", ";
    ofs_ << GlobName(symbol.name) << "@PAGEOFF" << std::endl;

    return reg;
}

/*
 * Read the value a pointer points at into the same register that holds the
 * pointer. How much is read is decided by what the pointer points at.
 */
reg_idx Arm64Codegen::Deref(reg_idx reg, PrimitiveType pointer_type)
{
    switch (ValueAt(pointer_type))
    {
        case PrimitiveType::kChar:
            ofs_ << "\tldrb\t" << wreg_list_[reg] << ", [";
            ofs_ << xreg_list_[reg] << "]" << std::endl;
            break;
        case PrimitiveType::kInt:
            ofs_ << "\tldr\t" << wreg_list_[reg] << ", [";
            ofs_ << xreg_list_[reg] << "]" << std::endl;
            break;
        case PrimitiveType::kLong:
            ofs_ << "\tldr\t" << xreg_list_[reg] << ", [";
            ofs_ << xreg_list_[reg] << "]" << std::endl;
            break;
        default:
            std::cerr << "Error: cannot read through this pointer!" << std::endl;
            std::exit(1);
    }

    return reg;
}

std::unique_ptr<AsmCodegen> MakeCodegen(const std::string& output_file)
{
    return std::make_unique<Arm64Codegen>(output_file);
}

}   /* namespace nuocc */
