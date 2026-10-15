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
    const std::vector<Symbol>& parameters)
{
    ofs_ << "\t.text" << std::endl;

    /* A function declared static belongs to this file alone, so the
     * assembler is not told that another file may reach it. */
    if (!symbol.is_static)
        ofs_ << "\t.globl\t" << GlobName(symbol.name) << std::endl;

    ofs_ << GlobName(symbol.name) << ":" << std::endl;

    /* The stack has to stay a multiple of sixteen bytes at every call. */
    const int32 spill_size = (local_size + 7) / 8 * 8;

    frame_size_ = ((kBaseFrameSize + spill_size + kSpillAreaSize + 15) / 16)
                  * 16;
    spill_base_ = kBaseFrameSize + spill_size;
    ofs_ << "\tsub\tsp, sp, #" << frame_size_ << std::endl;
    ofs_ << "\tstp\tx29, x30, [sp]" << std::endl;
    ofs_ << "\tmov\tx29, sp" << std::endl;
    ofs_ << "\tstp\t" << xreg_list_[0] << ", " << xreg_list_[1];
    ofs_ << ", [sp, #" << kSavedRegs1Offset << "]" << std::endl;
    ofs_ << "\tstp\t" << xreg_list_[2] << ", " << xreg_list_[3];
    ofs_ << ", [sp, #" << kSavedRegs2Offset << "]" << std::endl;

    /*
     * Each parameter arrives in x0 to x5 and is kept in the stack frame of
     * the function, where the body reads it as the local variable it is.
     */
    for (idx_t k = 0; k < parameters.size(); k++)
    {
        const Symbol& parameter = parameters[k];
        const int32 offset = kBaseFrameSize + parameter.stack_offset;
        const std::string reg64 = "x" + std::to_string(k);
        const std::string reg32 = "w" + std::to_string(k);

        switch (PrimitiveSize(parameter.type))
        {
            case 1:
                ofs_ << "\tstrb\t" << reg32 << ", [x29, #" << offset;
                ofs_ << "]" << std::endl;
                break;
            case 4:
                ofs_ << "\tstr\t" << reg32 << ", [x29, #" << offset;
                ofs_ << "]" << std::endl;
                break;
            case 8:
                ofs_ << "\tstr\t" << reg64 << ", [x29, #" << offset;
                ofs_ << "]" << std::endl;
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
                ofs_ << "\tldrsw\t" << xreg_list_[idx] << ", [x29, #";
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
     * How much is read is decided by the type's size, and the whole
     * register ends up holding the value. A char is unsigned, so it is
     * read with the zero extending load; an int is signed, and reading it
     * without the sign would turn a negative value into a large positive
     * one.
     */
    switch (PrimitiveSize(symbol.type))
    {
        case 1:
            ofs_ << "\tldrb\t" << wreg_list_[idx] << ", [" << kScratchReg;
            ofs_ << "]" << std::endl;
            break;
        case 4:
            ofs_ << "\tldrsw\t" << xreg_list_[idx] << ", [";
            ofs_ << kScratchReg << "]" << std::endl;
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
    ofs_ << "\tadd\t" << xreg_list_[reg1] << ", " << xreg_list_[reg1];
    ofs_ << ", " << xreg_list_[reg2] << std::endl;

    /* The register which was handed out last is the one whose value is
     * done with first, so that a value set aside to make room for it comes
     * back before anything else is worked out. */
    FreeRegister(reg2);

    return reg1;
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
    ofs_ << "\tmul\t" << xreg_list_[reg1] << ", " << xreg_list_[reg1];
    ofs_ << ", " << xreg_list_[reg2] << std::endl;

    /* The register which was handed out last is the one whose value is
     * done with first, so that a value set aside to make room for it comes
     * back before anything else is worked out. */
    FreeRegister(reg2);

    return reg1;
}

reg_idx Arm64Codegen::Div(reg_idx reg1, reg_idx reg2)
{
    ofs_ << "\tsdiv\t" << xreg_list_[reg1] << ", " << xreg_list_[reg1];
    ofs_ << ", " << xreg_list_[reg2] << std::endl;

    FreeRegister(reg2);

    return reg1;
}

/*
 * The bitwise operations. Each of them writes the result over reg2 and
 * gives reg1 back, exactly like Add() and the rest.
 */
reg_idx Arm64Codegen::And(reg_idx reg1, reg_idx reg2)
{
    ofs_ << "\tand\t" << xreg_list_[reg1] << ", " << xreg_list_[reg1];
    ofs_ << ", " << xreg_list_[reg2] << std::endl;

    /* The register which was handed out last is the one whose value is
     * done with first, so that a value set aside to make room for it comes
     * back before anything else is worked out. */
    FreeRegister(reg2);

    return reg1;
}

reg_idx Arm64Codegen::Or(reg_idx reg1, reg_idx reg2)
{
    ofs_ << "\torr\t" << xreg_list_[reg1] << ", " << xreg_list_[reg1];
    ofs_ << ", " << xreg_list_[reg2] << std::endl;

    /* The register which was handed out last is the one whose value is
     * done with first, so that a value set aside to make room for it comes
     * back before anything else is worked out. */
    FreeRegister(reg2);

    return reg1;
}

reg_idx Arm64Codegen::Xor(reg_idx reg1, reg_idx reg2)
{
    ofs_ << "\teor\t" << xreg_list_[reg1] << ", " << xreg_list_[reg1];
    ofs_ << ", " << xreg_list_[reg2] << std::endl;

    /* The register which was handed out last is the one whose value is
     * done with first, so that a value set aside to make room for it comes
     * back before anything else is worked out. */
    FreeRegister(reg2);

    return reg1;
}

/*
 * A shift takes the number of bits to shift by in the low bits of a
 * register, which is where that value already is.
 */
reg_idx Arm64Codegen::ShiftLeft(reg_idx reg1, reg_idx reg2)
{
    ofs_ << "\tlsl\t" << xreg_list_[reg1] << ", " << xreg_list_[reg1];
    ofs_ << ", " << xreg_list_[reg2] << std::endl;

    /* The register which was handed out last is the one whose value is
     * done with first, so that a value set aside to make room for it comes
     * back before anything else is worked out. */
    FreeRegister(reg2);

    return reg1;
}

reg_idx Arm64Codegen::ShiftRight(reg_idx reg1, reg_idx reg2)
{
    /* 'lsr' shifts zeroes in, 'asr' would keep the sign. */
    ofs_ << "\tlsr\t" << xreg_list_[reg1] << ", " << xreg_list_[reg1];
    ofs_ << ", " << xreg_list_[reg2] << std::endl;

    /* The register which was handed out last is the one whose value is
     * done with first, so that a value set aside to make room for it comes
     * back before anything else is worked out. */
    FreeRegister(reg2);

    return reg1;
}

void Arm64Codegen::Move(reg_idx dest, reg_idx src)
{
    ofs_ << "\tmov\t" << xreg_list_[dest] << ", " << xreg_list_[src];
    ofs_ << std::endl;
}

reg_idx Arm64Codegen::Negate(reg_idx reg)
{
    ofs_ << "\tneg\t" << xreg_list_[reg] << ", " << xreg_list_[reg];
    ofs_ << std::endl;

    return reg;
}

reg_idx Arm64Codegen::Invert(reg_idx reg)
{
    ofs_ << "\tmvn\t" << xreg_list_[reg] << ", " << xreg_list_[reg];
    ofs_ << std::endl;

    return reg;
}

/*
 * A value is false when it is zero and true otherwise. 'cmp' against the
 * zero register sets the flags, and 'cset' then writes the answer, which
 * is a clean zero or one on its own.
 */
reg_idx Arm64Codegen::LogNot(reg_idx reg)
{
    ofs_ << "\tcmp\t" << xreg_list_[reg] << ", xzr" << std::endl;
    ofs_ << "\tcset\t" << xreg_list_[reg] << ", eq" << std::endl;

    return reg;
}

reg_idx Arm64Codegen::ToBool(reg_idx reg)
{
    ofs_ << "\tcmp\t" << xreg_list_[reg] << ", xzr" << std::endl;
    ofs_ << "\tcset\t" << xreg_list_[reg] << ", ne" << std::endl;

    return reg;
}

/*
 * AArch64 has no instruction which alters a value in memory, so the value
 * is loaded, altered and stored back, which needs a second register to
 * hold the new value while the old one is still around.
 */
reg_idx Arm64Codegen::IncDec(const Symbol& symbol, int32 delta, bool post)
{
    int32 amount = delta > 0 ? delta : -delta;

    reg_idx old_reg = LoadSymbol(symbol);
    reg_idx new_reg = AllocRegister();

    ofs_ << "\t" << (delta > 0 ? "add" : "sub") << "\t";
    ofs_ << xreg_list_[new_reg] << ", " << xreg_list_[old_reg];
    ofs_ << ", #" << amount << std::endl;

    StoreSymbol(symbol, new_reg);

    /*
     * Written after the variable the expression yields the value it held
     * before the change, written before it the value it holds now.
     */
    if (post)
    {
        FreeRegister(new_reg);
        return old_reg;
    }

    FreeRegister(old_reg);

    return new_reg;
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
reg_idx Arm64Codegen::Cast(reg_idx reg,
    PrimitiveType /*old_type*/,
    PrimitiveType new_type)
{
    /*
     * A pointer and a long are as wide as the register, so there is nothing
     * of them to cut down. A char and an int are narrower, and the value
     * which is cast to one of them keeps the bits it can hold: the low
     * eight for a char and the low thirty two for an int. Writing a w
     * register already leaves the top of the x register clear.
     */
    switch (new_type)
    {
        case PrimitiveType::kChar:
            ofs_ << "\tand\t" << wreg_list_[reg] << ", " << wreg_list_[reg];
            ofs_ << ", #0xff" << std::endl;
            break;
        case PrimitiveType::kInt:
            ofs_ << "\tmov\t" << wreg_list_[reg] << ", " << wreg_list_[reg];
            ofs_ << std::endl;
            break;
        default:
            break;
    }

    return reg;
}

void Arm64Codegen::SpillRegister(reg_idx reg, int32 slot)
{
    ofs_ << "\tstr\t" << xreg_list_[reg] << ", [x29, #";
    ofs_ << spill_base_ + 8 * slot << "]" << std::endl;
}

void Arm64Codegen::ReloadRegister(reg_idx reg, int32 slot)
{
    ofs_ << "\tldr\t" << xreg_list_[reg] << ", [x29, #";
    ofs_ << spill_base_ + 8 * slot << "]" << std::endl;
}

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
    ofs_ << "\tcset\t" << xreg_list_[reg1] << ", ";
    ofs_ << it->second.condition << std::endl;

    FreeRegister(reg2);

    return reg1;
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

    /* The comparison is a jump and answers with nothing, so both values it
     * was given are done with. Only those two are freed: what the rest of
     * the expression holds is not this comparison's to throw away. */
    FreeRegister(reg1);
    FreeRegister(reg2);
}

/*
 * Jump when the value in a register is zero, which AArch64 can do in one
 * instruction of its own.
 */
void Arm64Codegen::JumpIfEqual(reg_idx reg1,
    reg_idx reg2,
    label_idx label)
{
    ofs_ << "\tcmp\t" << xreg_list_[reg1] << ", " << xreg_list_[reg2];
    ofs_ << std::endl;
    ofs_ << "\tb.eq\tL" << label << std::endl;
}

void Arm64Codegen::JumpIfZero(reg_idx reg, label_idx label)
{
    ofs_ << "\tcbz\t" << xreg_list_[reg] << ", L" << label << std::endl;
}

void Arm64Codegen::JumpIfNonZero(reg_idx reg, label_idx label)
{
    ofs_ << "\tcbnz\t" << xreg_list_[reg] << ", L" << label << std::endl;
}

/*
 * Put one argument on the stack while a call is being made. Every value
 * takes sixteen bytes, so the stack stays aligned to sixteen bytes while
 * the remaining arguments are worked out, which is what a function called
 * from one of them needs.
 */
void Arm64Codegen::PushArgument(reg_idx reg)
{
    ofs_ << "\tsub\tsp, sp, #16" << std::endl;
    ofs_ << "\tstr\t" << xreg_list_[reg] << ", [sp]" << std::endl;
}

/*
 * Call a function with the argument values which have been pushed, the
 * first of them nearest to the stack pointer. Up to six of them travel in
 * x0 to x5, and the ones beyond that would have to travel on the stack.
 *
 * The value registers are callee saved, so whatever an expression is still
 * holding survives the call on its own.
 */
reg_idx Arm64Codegen::Call(const Symbol& symbol, int32 arg_count)
{
    for (int32 arg = 0; arg < arg_count; arg++)
    {
        ofs_ << "\tldr\tx" << arg << ", [sp, #" << 16 * arg << "]";
        ofs_ << std::endl;
    }

    if (arg_count > 0)
        ofs_ << "\tadd\tsp, sp, #" << 16 * arg_count << std::endl;

    ofs_ << "\tbl\t" << GlobName(symbol.name) << std::endl;

    reg_idx out_reg = AllocRegister();

    if (symbol.type == PrimitiveType::kInt) {
        ofs_ << "\tsxtw\t" << xreg_list_[out_reg] << ", w0\n";
    } else if (symbol.type == PrimitiveType::kChar) {
        ofs_ << "\tuxtb\t" << wreg_list_[out_reg] << ", w0\n";
    } else {
        ofs_ << "\tmov\t" << xreg_list_[out_reg] << ", x0\n";
    }

    return out_reg;
}

/*
 * Return from the function being generated. The result goes in x0, where
 * the caller expects to find it, and control jumps to the end label.
 */
void Arm64Codegen::Return(reg_idx reg)
{
    if (IsPointerType(function_return_type_)) {
        ofs_ << "\tmov\tx0, " << xreg_list_[reg] << '\n';
        EmitJump(function_end_label_);
        return;
    }
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

void Arm64Codegen::AddOffset(/*
 * The address of a string literal is reached the same way as a global
 * variable's, but a label is local, so it takes no GlobName() spelling.
 */
reg_idx reg, int32 offset) {
    if (offset == 0)
        return;
    if (offset <= 4095) {
        ofs_ << "\tadd\t" << xreg_list_[reg] << ", " << xreg_list_[reg]
             << ", #" << offset << '\n';
        return;
    }
    ofs_ << "\tmovz\t" << kScratchReg << ", #" << (offset & 0xFFFF) << '\n';
    ofs_ << "\tmovk\t" << kScratchReg << ", #" << (offset >> 16)
         << ", lsl #16\n";
    ofs_ << "\tadd\t" << xreg_list_[reg] << ", " << xreg_list_[reg]
         << ", " << kScratchReg << '\n';
}

reg_idx Arm64Codegen::LoadStrAddress(label_idx label)
{
    const std::string name = "L" + std::to_string(label);
    reg_idx reg = AllocRegister();

    ofs_ << "\tadrp\t" << xreg_list_[reg] << ", " << name << "@PAGE";
    ofs_ << std::endl;
    ofs_ << "\tadd\t" << xreg_list_[reg] << ", " << xreg_list_[reg] << ", ";
    ofs_ << name << "@PAGEOFF" << std::endl;

    return reg;
}

/*
 * Read the value a pointer points at into the same register that holds the
 * pointer. How much is read is decided by what the pointer points at.
 */
reg_idx Arm64Codegen::Deref(reg_idx reg, PrimitiveType pointer_type)
{
    /*
     * How much is read is decided by the size of what the pointer points
     * at, which is what makes reading through a pointer to a pointer read
     * a whole address.
     */
    switch (PrimitiveSize(ValueAt(pointer_type)))
    {
        case 1:
            ofs_ << "\tldrb\t" << wreg_list_[reg] << ", [";
            ofs_ << xreg_list_[reg] << "]" << std::endl;
            break;
        case 4:
            ofs_ << "\tldrsw\t" << xreg_list_[reg] << ", [";
            ofs_ << xreg_list_[reg] << "]" << std::endl;
            break;
        case 8:
            ofs_ << "\tldr\t" << xreg_list_[reg] << ", [";
            ofs_ << xreg_list_[reg] << "]" << std::endl;
            break;
        default:
            std::cerr << "Error: cannot read through this pointer!" << std::endl;
            std::exit(1);
    }

    return reg;
}

/*
 * Write a value through a pointer, the other way round from Deref(). How
 * much is written is decided by the type of the value, so that storing
 * into a char does not run over the value stored next to it.
 */
reg_idx Arm64Codegen::LoadDeref(reg_idx address_reg, PrimitiveType type)
{
    reg_idx value_reg = AllocRegister();

    switch (PrimitiveSize(type))
    {
        case 1:
            ofs_ << "\tldrb\t" << wreg_list_[value_reg] << ", [";
            break;
        case 4:
            ofs_ << "\tldrsw\t" << xreg_list_[value_reg] << ", [";
            break;
        case 8:
            ofs_ << "\tldr\t" << xreg_list_[value_reg] << ", [";
            break;
        default:
            std::cerr << "Error: cannot read through this pointer!" << std::endl;
            std::exit(1);
    }

    ofs_ << xreg_list_[address_reg] << "]" << std::endl;

    return value_reg;
}

void Arm64Codegen::StoreDeref(reg_idx value_reg,
    reg_idx address_reg,
    PrimitiveType type)
{
    switch (PrimitiveSize(type))
    {
        case 1:
            ofs_ << "\tstrb\t" << wreg_list_[value_reg] << ", [";
            break;
        case 4:
            ofs_ << "\tstr\t" << wreg_list_[value_reg] << ", [";
            break;
        case 8:
            ofs_ << "\tstr\t" << xreg_list_[value_reg] << ", [";
            break;
        default:
            std::cerr << "Error: cannot write through this pointer!";
            std::cerr << std::endl;
            std::exit(1);
    }

    ofs_ << xreg_list_[address_reg] << "]" << std::endl;
}

std::unique_ptr<AsmCodegen> MakeCodegen(const std::string& output_file)
{
    return std::make_unique<Arm64Codegen>(output_file);
}

}   /* namespace nuocc */
