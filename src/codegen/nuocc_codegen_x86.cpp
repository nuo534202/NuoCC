#include "codegen/nuocc_codegen_x86.hpp"

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

/*
 * The registers the SysV ABI passes arguments in, written in each of the
 * widths a parameter may be stored with. An argument beyond the sixth
 * would have to travel on the stack, which the language does not do yet.
 */
constexpr std::string_view kParamRegs64[] = {
    "%rdi", "%rsi", "%rdx", "%rcx", "%r8", "%r9"
};
constexpr std::string_view kParamRegs32[] = {
    "%edi", "%esi", "%edx", "%ecx", "%r8d", "%r9d"
};
constexpr std::string_view kParamRegs8[] = {
    "%dil", "%sil", "%dl", "%cl", "%r8b", "%r9b"
};

}   /* namespace */

X86Codegen::X86Codegen(const std::string& output_file)
    : AssemblyCodegen(output_file)
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

void X86Codegen::EmitFunctionPreamble(
    const Symbol& symbol,
    int32 local_size,
    const std::vector<Symbol>& parameters)
{
    ofs_ << "\t.text" << std::endl;

    /* A function declared static belongs to this file alone, so the
     * assembler is not told that another file may reach it. */
    if (!symbol.is_static)
        ofs_ << "\t.globl\t" << symbol.name << std::endl;

    ofs_ << "\t.type\t" << symbol.name << ", @function" << std::endl;
    ofs_ << symbol.name << ":" << std::endl;
    ofs_ << "\tpushq\t%rbp" << std::endl;
    ofs_ << "\tmovq\t%rsp, %rbp" << std::endl;

    spill_base_ = (local_size + 7) / 8 * 8;

    const int32 frame_size =
        (spill_base_ + kSpillAreaSize + 15) / 16 * 16;
    if (frame_size != 0)
        ofs_ << "\tsubq\t$" << frame_size << ", %rsp" << std::endl;

    /*
     * Each parameter arrives in the register the ABI gives it and is kept
     * in the stack frame of the function, where the body reads it as the
     * local variable it is.
     */
    for (idx_t k = 0; k < parameters.size(); k++)
    {
        const Symbol& parameter = parameters[k];
        const int32 offset = parameter.stack_offset +
                             PrimitiveSize(parameter.type);

        switch (PrimitiveSize(parameter.type))
        {
            case 1:
                ofs_ << "\tmovb\t" << kParamRegs8[k] << ", -";
                ofs_ << offset << "(%rbp)" << std::endl;
                break;
            case 4:
                ofs_ << "\tmovl\t" << kParamRegs32[k] << ", -";
                ofs_ << offset << "(%rbp)" << std::endl;
                break;
            case 8:
                ofs_ << "\tmovq\t" << kParamRegs64[k] << ", -";
                ofs_ << offset << "(%rbp)" << std::endl;
                break;
            default:
                std::cerr << "Error: invalid parameter type!" << std::endl;
                std::exit(1);
        }
    }
}

void X86Codegen::EmitFunctionPostamble()
{
    EmitLabel(function_end_label_);

    /*
     * The frame pointer is where the stack pointer was before the frame of
     * the function was made, so it has to be given back before the saved
     * frame pointer is popped: a function which made room for locals and
     * popped without doing this would return to whatever its own frame
     * happens to hold.
     */
    ofs_ << "\tmovq\t%rbp, %rsp" << std::endl;
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

reg_idx X86Codegen::LoadInt(int32 value, PrimitiveType /*type*/)
{
    reg_idx reg = AllocRegister();

    ofs_ << "\tmovq\t$" << value << ", " << reg_list_[reg] << std::endl;

    return reg;
}

reg_idx X86Codegen::LoadSymbol(const Symbol& symbol)
{
    reg_idx idx = AllocRegister();

    /*
     * How much is read is decided by the type's size, and every one of
     * these leaves the whole register holding the value. A char is
     * unsigned, so it is read with the zero extending load; an int is
     * signed, and reading it without the sign would turn a negative value
     * into a large positive one.
     */
    if (symbol.storage == StorageClass::kLocal)
    {
        const int32 offset = symbol.stack_offset + PrimitiveSize(symbol.type);
        switch (PrimitiveSize(symbol.type))
        {
            case 1:
                ofs_ << "\tmovzbq\t-" << offset << "(%rbp), ";
                ofs_ << reg_list_[idx] << std::endl;
                break;
            case 4:
                ofs_ << "\tmovslq\t-" << offset << "(%rbp), ";
                ofs_ << reg_list_[idx] << std::endl;
                break;
            case 8:
                ofs_ << "\tmovq\t-" << offset << "(%rbp), ";
                ofs_ << reg_list_[idx] << std::endl;
                break;
            default:
                std::cerr << "Error: bad type for variable ";
                std::cerr << symbol.name << "!" << std::endl;
                std::exit(1);
        }

        return idx;
    }

    switch (PrimitiveSize(symbol.type))
    {
        case 1:
            ofs_ << "\tmovzbq\t" << symbol.name << "(%rip), ";
            ofs_ << reg_list_[idx] << std::endl;
            break;
        case 4:
            ofs_ << "\tmovslq\t" << symbol.name << "(%rip), ";
            ofs_ << reg_list_[idx] << std::endl;
            break;
        case 8:
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

reg_idx X86Codegen::StoreSymbol(const Symbol& symbol, reg_idx reg)
{
    if (symbol.storage == StorageClass::kLocal)
    {
        const int32 offset = symbol.stack_offset + PrimitiveSize(symbol.type);
        switch (PrimitiveSize(symbol.type))
        {
            case 1:
                ofs_ << "\tmovb\t" << breg_list_[reg] << ", -";
                break;
            case 4:
                ofs_ << "\tmovl\t" << dreg_list_[reg] << ", -";
                break;
            case 8:
                ofs_ << "\tmovq\t" << reg_list_[reg] << ", -";
                break;
            default:
                std::cerr << "Error: bad type for variable ";
                std::cerr << symbol.name << "!" << std::endl;
                std::exit(1);
        }
        ofs_ << offset << "(%rbp)" << std::endl;
        return reg;
    }

    switch (PrimitiveSize(symbol.type))
    {
        case 1:
            ofs_ << "\tmovb\t" << breg_list_[reg] << ", ";
            break;
        case 4:
            ofs_ << "\tmovl\t" << dreg_list_[reg] << ", ";
            break;
        case 8:
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

reg_idx X86Codegen::Add(reg_idx reg1, reg_idx reg2, PrimitiveType /*type*/)
{
    ofs_ << "\taddq\t" << reg_list_[reg2] << ", ";
    ofs_ << reg_list_[reg1] << std::endl;

    /* The register which was handed out last is the one whose value is
     * done with first, so that a value set aside to make room for it comes
     * back before anything else is worked out. */
    FreeRegister(reg2);

    return reg1;
}

reg_idx X86Codegen::Sub(reg_idx reg1, reg_idx reg2, PrimitiveType /*type*/)
{
    ofs_ << "\tsubq\t" << reg_list_[reg2] << ", ";
    ofs_ << reg_list_[reg1] << std::endl;

    FreeRegister(reg2);

    return reg1;
}

reg_idx X86Codegen::Mul(reg_idx reg1, reg_idx reg2, PrimitiveType /*type*/)
{
    ofs_ << "\timulq\t" << reg_list_[reg2] << ", ";
    ofs_ << reg_list_[reg1] << std::endl;

    /* The register which was handed out last is the one whose value is
     * done with first, so that a value set aside to make room for it comes
     * back before anything else is worked out. */
    FreeRegister(reg2);

    return reg1;
}

reg_idx X86Codegen::Div(reg_idx reg1, reg_idx reg2, PrimitiveType /*type*/)
{
    ofs_ << "\tmovq\t" << reg_list_[reg1] << ", %rax" << std::endl;
    ofs_ << "\tcqo" << std::endl;
    ofs_ << "\tidivq\t" << reg_list_[reg2] << std::endl;
    ofs_ << "\tmovq\t%rax, " << reg_list_[reg1] << std::endl;

    FreeRegister(reg2);

    return reg1;
}

/*
 * The bitwise operations. Each of them writes the result over reg2 and
 * gives reg1 back, exactly like Add() and the rest.
 */
reg_idx X86Codegen::And(reg_idx reg1, reg_idx reg2, PrimitiveType /*type*/)
{
    ofs_ << "\tandq\t" << reg_list_[reg2] << ", ";
    ofs_ << reg_list_[reg1] << std::endl;

    /* The register which was handed out last is the one whose value is
     * done with first, so that a value set aside to make room for it comes
     * back before anything else is worked out. */
    FreeRegister(reg2);

    return reg1;
}

reg_idx X86Codegen::Or(reg_idx reg1, reg_idx reg2, PrimitiveType /*type*/)
{
    ofs_ << "\torq\t" << reg_list_[reg2] << ", ";
    ofs_ << reg_list_[reg1] << std::endl;

    /* The register which was handed out last is the one whose value is
     * done with first, so that a value set aside to make room for it comes
     * back before anything else is worked out. */
    FreeRegister(reg2);

    return reg1;
}

reg_idx X86Codegen::Xor(reg_idx reg1, reg_idx reg2, PrimitiveType /*type*/)
{
    ofs_ << "\txorq\t" << reg_list_[reg2] << ", ";
    ofs_ << reg_list_[reg1] << std::endl;

    /* The register which was handed out last is the one whose value is
     * done with first, so that a value set aside to make room for it comes
     * back before anything else is worked out. */
    FreeRegister(reg2);

    return reg1;
}

/*
 * A shift takes the number of bits to shift by in %cl, so the value in
 * reg2 is moved there first. The result is left in reg1, the register
 * which held the value being shifted.
 */
reg_idx X86Codegen::ShiftLeft(reg_idx reg1, reg_idx reg2, PrimitiveType /*type*/)
{
    ofs_ << "\tmovb\t" << breg_list_[reg2] << ", %cl" << std::endl;
    ofs_ << "\tshlq\t%cl, " << reg_list_[reg1] << std::endl;

    FreeRegister(reg2);

    return reg1;
}

reg_idx X86Codegen::ShiftRight(reg_idx reg1, reg_idx reg2, PrimitiveType /*type*/)
{
    ofs_ << "\tmovb\t" << breg_list_[reg2] << ", %cl" << std::endl;
    ofs_ << "\tshrq\t%cl, " << reg_list_[reg1] << std::endl;

    FreeRegister(reg2);

    return reg1;
}

reg_idx X86Codegen::Mod(reg_idx reg1, reg_idx reg2, PrimitiveType /*type*/)
{
    /* idivq leaves the quotient in %rax and what is left over in %rdx. */
    ofs_ << "\tmovq\t" << reg_list_[reg1] << ", %rax" << std::endl;
    ofs_ << "\tcqo" << std::endl;
    ofs_ << "\tidivq\t" << reg_list_[reg2] << std::endl;
    ofs_ << "\tmovq\t%rdx, " << reg_list_[reg1] << std::endl;

    FreeRegister(reg2);

    return reg1;
}

void X86Codegen::Move(reg_idx dest, reg_idx src, PrimitiveType /*type*/)
{
    ofs_ << "\tmovq\t" << reg_list_[src] << ", " << reg_list_[dest];
    ofs_ << std::endl;
}

reg_idx X86Codegen::Negate(reg_idx reg, PrimitiveType /*type*/)
{
    ofs_ << "\tnegq\t" << reg_list_[reg] << std::endl;

    return reg;
}

reg_idx X86Codegen::Invert(reg_idx reg, PrimitiveType /*type*/)
{
    ofs_ << "\tnotq\t" << reg_list_[reg] << std::endl;

    return reg;
}

/*
 * A value is false when it is zero and true otherwise. test sets the flags
 * from the value anded with itself, and sete and setnz then write the
 * answer into the low byte, which movzbq extends to a clean 0 or 1.
 */
reg_idx X86Codegen::LogNot(reg_idx reg, PrimitiveType /*type*/)
{
    ofs_ << "\ttest\t" << reg_list_[reg] << ", ";
    ofs_ << reg_list_[reg] << std::endl;
    ofs_ << "\tsete\t" << breg_list_[reg] << std::endl;
    ofs_ << "\tmovzbq\t" << breg_list_[reg] << ", ";
    ofs_ << reg_list_[reg] << std::endl;

    return reg;
}

reg_idx X86Codegen::ToBool(reg_idx reg, PrimitiveType /*type*/)
{
    ofs_ << "\ttest\t" << reg_list_[reg] << ", ";
    ofs_ << reg_list_[reg] << std::endl;
    ofs_ << "\tsetnz\t" << breg_list_[reg] << std::endl;
    ofs_ << "\tmovzbq\t" << breg_list_[reg] << ", ";
    ofs_ << reg_list_[reg] << std::endl;

    return reg;
}

/*
 * Add one to, or take one from, the value a variable holds in memory
 * without moving it into a register first, which x86-64 can do in one
 * instruction.
 */
void X86Codegen::EmitIncDecMemory(const Symbol& symbol, int32 delta)
{
    ofs_ << "\t" << (delta > 0 ? "inc" : "dec");

    switch (PrimitiveSize(symbol.type))
    {
        case 1:
            ofs_ << "b";
            break;
        case 4:
            ofs_ << "l";
            break;
        case 8:
            ofs_ << "q";
            break;
        default:
            std::cerr << "Error: bad type for variable ";
            std::cerr << symbol.name << "!" << std::endl;
            std::exit(1);
    }

    ofs_ << "\t";

    if (symbol.storage == StorageClass::kLocal)
    {
        const int32 offset = symbol.stack_offset + PrimitiveSize(symbol.type);
        ofs_ << "-" << offset << "(%rbp)" << std::endl;
        return;
    }

    ofs_ << symbol.name << "(%rip)" << std::endl;
}

reg_idx X86Codegen::IncDec(const Symbol& symbol, int32 delta, bool post)
{
    /*
     * Which value the expression yields is the only difference between the
     * two sides: written after the variable it yields the value held
     * before the change, written before it the value held afterwards.
     */
    if (post)
    {
        reg_idx reg = LoadSymbol(symbol);

        EmitIncDecMemory(symbol, delta);

        return reg;
    }

    EmitIncDecMemory(symbol, delta);

    return LoadSymbol(symbol);
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

/*
 * Cut the value in a register down to the type it is cast to, so that what
 * the register holds is a value of the new type.
 */
reg_idx X86Codegen::Cast(reg_idx reg,
    PrimitiveType /*old_type*/,
    PrimitiveType new_type)
{
    /*
     * A pointer and a long are as wide as the register, so there is nothing
     * of them to cut down. A char and an int are narrower, and the value
     * which is cast to one of them keeps the bits it can hold: the low
     * eight for a char and the low thirty two for an int. A value of the
     * wider type is a number of its own, so the bits it keeps are read back
     * as that number: a char has no negative ones, while an int does, and
     * the top of the register is filled in to say so.
     */
    switch (new_type)
    {
        case PrimitiveType::kChar:
            ofs_ << "\tmovzbl\t" << breg_list_[reg] << ", ";
            ofs_ << dreg_list_[reg] << std::endl;
            break;
        case PrimitiveType::kInt:
            ofs_ << "\tmovslq\t" << dreg_list_[reg] << ", ";
            ofs_ << reg_list_[reg] << std::endl;
            break;
        default:
            break;
    }

    return reg;
}

void X86Codegen::SpillRegister(reg_idx reg, int32 slot)
{
    ofs_ << "\tmovq\t" << reg_list_[reg] << ", -";
    ofs_ << spill_base_ + 8 * (slot + 1) << "(%rbp)" << std::endl;
}

void X86Codegen::ReloadRegister(reg_idx reg, int32 slot)
{
    ofs_ << "\tmovq\t-" << spill_base_ + 8 * (slot + 1) << "(%rbp), ";
    ofs_ << reg_list_[reg] << std::endl;
}

/*
 * Multiply the value in a register by the size of a type, which is what
 * turns an index into an offset from the base of what is indexed. Every
 * size the language has now is a power of two, so a shift left is enough
 * and cheaper than a multiply; the multiply is there for the sizes which
 * are not, which composite types will bring.
 */
reg_idx X86Codegen::Scale(reg_idx reg, int32 scale, PrimitiveType type)
{
    uint32 size = static_cast<uint32>(scale);

    if (std::has_single_bit(size))
    {
        ofs_ << "\tsalq\t$" << std::countr_zero(size) << ", ";
        ofs_ << reg_list_[reg] << std::endl;

        return reg;
    }

    /* The size of a value is a value of the type being scaled. */
    reg_idx size_reg = LoadInt(scale, type);

    return Mul(reg, size_reg, type);
}

reg_idx X86Codegen::CompareAndSet(NodeTag op_type, reg_idx reg1, reg_idx reg2, PrimitiveType /*type*/)
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
    ofs_ << "\t" << it->second.set << "\t" << breg_list_[reg1] << std::endl;
    ofs_ << "\tmovzbq\t" << breg_list_[reg1] << ", ";
    ofs_ << reg_list_[reg1] << std::endl;

    /* The register which was handed out last is the one whose value is
     * done with first, so that a value set aside to make room for it comes
     * back before anything else is worked out. */
    FreeRegister(reg2);

    return reg1;
}

void X86Codegen::CompareAndJump(NodeTag op_type,
    reg_idx reg1,
    reg_idx reg2,
    label_idx label,
    PrimitiveType /*type*/)
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

    /* The comparison is a jump and answers with nothing, so both values it
     * was given are done with. Only those two are freed: what the rest of
     * the expression holds is not this comparison's to throw away. */
    FreeRegister(reg1);
    FreeRegister(reg2);
}

/*
 * Jump when the value in a register is zero. test sets the flags from the
 * value anded with itself, and je is taken when that result was zero.
 */
void X86Codegen::JumpIfEqual(reg_idx reg1,
    reg_idx reg2,
    label_idx label,
    PrimitiveType /*type*/)
{
    ofs_ << "\tcmpq\t" << reg_list_[reg2] << ", " << reg_list_[reg1];
    ofs_ << std::endl;
    ofs_ << "\tje\tL" << label << std::endl;
}

void X86Codegen::JumpIfZero(reg_idx reg, label_idx label)
{
    ofs_ << "\ttest\t" << reg_list_[reg] << ", ";
    ofs_ << reg_list_[reg] << std::endl;
    ofs_ << "\tje\tL" << label << std::endl;
}

void X86Codegen::JumpIfNonZero(reg_idx reg, label_idx label)
{
    ofs_ << "\ttest\t" << reg_list_[reg] << ", ";
    ofs_ << reg_list_[reg] << std::endl;
    ofs_ << "\tjne\tL" << label << std::endl;
}

/*
 * Put one argument on the stack while a call is being made. Every value
 * takes sixteen bytes, so the stack stays aligned to sixteen bytes while
 * the remaining arguments are worked out, which is what a function called
 * from one of them needs.
 */
void X86Codegen::PushArgument(reg_idx reg, PrimitiveType /*type*/)
{
    ofs_ << "\tsubq\t$16, %rsp" << std::endl;
    ofs_ << "\tmovq\t" << reg_list_[reg] << ", (%rsp)" << std::endl;
}

/*
 * Call a function with the argument values which have been pushed, the
 * first of them nearest to the stack pointer. Up to six arguments travel
 * in the registers the ABI gives them, and the ones beyond that would have
 * to travel on the stack.
 *
 * The call is allowed to clobber every register, so the ones which still
 * hold a value have to be saved across it. An odd number of saved
 * registers would leave the stack misaligned for the call, so it is padded
 * back to a multiple of sixteen bytes; the argument slots are already
 * sixteen bytes each and need no padding.
 */
reg_idx X86Codegen::Call(const Symbol& symbol, int32 arg_count)
{
    const bool *is_free = FreeRegisters();
    int32 saved = 0;

    for (reg_idx reg = 0; reg < kRegSize; reg++)
    {
        if (!is_free[reg])
        {
            ofs_ << "\tpushq\t" << reg_list_[reg] << std::endl;
            saved++;
        }
    }

    const int32 pad = saved % 2;
    if (pad != 0)
        ofs_ << "\tsubq\t$8, %rsp" << std::endl;

    /*
     * The saved registers and the padding sit between the stack pointer
     * and the arguments, so the offset of an argument is measured past
     * both of them. Loading the fifth and sixth arguments into %r8 and %r9
     * may overwrite a saved value there; the value was pushed above and is
     * given back once the call has returned.
     */
    for (int32 arg = 0; arg < arg_count; arg++)
    {
        const int32 offset = 8 * (saved + pad) + 16 * arg;

        ofs_ << "\tmovq\t" << offset << "(%rsp), ";
        ofs_ << kParamRegs64[arg] << std::endl;
    }

    ofs_ << "\tcall\t" << symbol.name << std::endl;

    if (pad != 0)
        ofs_ << "\taddq\t$8, %rsp" << std::endl;

    for (reg_idx reg = kRegSize; reg > 0; reg--)
    {
        if (!is_free[reg - 1])
            ofs_ << "\tpopq\t" << reg_list_[reg - 1] << std::endl;
    }

    if (arg_count > 0)
        ofs_ << "\taddq\t$" << 16 * arg_count << ", %rsp" << std::endl;

    reg_idx out_reg = AllocRegister();

    if (symbol.type == PrimitiveType::kInt) {
        ofs_ << "\tmovslq\t%eax, " << reg_list_[out_reg] << '\n';
    } else if (symbol.type == PrimitiveType::kChar) {
        ofs_ << "\tmovzbq\t%al, " << reg_list_[out_reg] << '\n';
    } else {
        ofs_ << "\tmovq\t%rax, " << reg_list_[out_reg] << '\n';
    }

    return out_reg;
}

/*
 * Return from the function being generated. The result goes in %rax, where
 * the caller expects to find it, and control jumps to the end label.
 */
void X86Codegen::Return(reg_idx reg)
{
    if (IsPointerType(function_return_type_)) {
        ofs_ << "\tmovq\t" << reg_list_[reg] << ", %rax\n";
        EmitJump(function_end_label_);
        return;
    }
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

/*
 * Load the address of a global variable into a fresh register.
 */
reg_idx X86Codegen::AddressOf(const Symbol& symbol)
{
    reg_idx reg = AllocRegister();

    if (symbol.storage == StorageClass::kLocal)
    {
        /*
         * The frame grows downwards from the frame pointer, so the address
         * asked for is the lowest one the symbol's room covers: a value
         * starts at the bottom of its room, and an array at the bottom of
         * all of its elements together.
         */
        const int32 offset = symbol.stack_offset + SymbolStorageSize(symbol);
        ofs_ << "\tleaq\t-" << offset << "(%rbp), ";
        ofs_ << reg_list_[reg] << std::endl;
        return reg;
    }

    ofs_ << "\tleaq\t" << symbol.name << "(%rip), ";
    ofs_ << reg_list_[reg] << std::endl;

    return reg;
}

void X86Codegen::AddOffset(/*
 * The address of a string literal is reached the same way as a global
 * variable's, but a label is local, so it takes no mangled spelling.
 */
reg_idx reg, int32 offset) {
    if (offset != 0)
        ofs_ << "\taddq\t$" << offset << ", " << reg_list_[reg] << '\n';
}

reg_idx X86Codegen::LoadStrAddress(label_idx label)
{
    const std::string name = "L" + std::to_string(label);
    reg_idx reg = AllocRegister();

    ofs_ << "\tleaq\t" << name << "(%rip), " << reg_list_[reg] << std::endl;

    return reg;
}

/*
 * Read the value a pointer points at into the same register that holds the
 * pointer. How much is read is decided by what the pointer points at.
 */
reg_idx X86Codegen::Deref(reg_idx reg, PrimitiveType pointer_type)
{
    /*
     * How much is read is decided by the size of what the pointer points
     * at, which is what makes reading through a pointer to a pointer read
     * a whole address.
     */
    switch (PrimitiveSize(ValueAt(pointer_type)))
    {
        case 1:
            ofs_ << "\tmovzbq\t(" << reg_list_[reg] << "), ";
            ofs_ << reg_list_[reg] << std::endl;
            break;
        case 4:
            ofs_ << "\tmovslq\t(" << reg_list_[reg] << "), ";
            ofs_ << reg_list_[reg] << std::endl;
            break;
        case 8:
            ofs_ << "\tmovq\t(" << reg_list_[reg] << "), ";
            ofs_ << reg_list_[reg] << std::endl;
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
reg_idx X86Codegen::LoadDeref(reg_idx address_reg, PrimitiveType type)
{
    reg_idx value_reg = AllocRegister();

    switch (PrimitiveSize(type))
    {
        case 1:
            ofs_ << "\tmovzbq\t(" << reg_list_[address_reg] << "), ";
            ofs_ << reg_list_[value_reg] << std::endl;
            break;
        case 4:
            ofs_ << "\tmovslq\t(" << reg_list_[address_reg] << "), ";
            ofs_ << reg_list_[value_reg] << std::endl;
            break;
        case 8:
            ofs_ << "\tmovq\t(" << reg_list_[address_reg] << "), ";
            ofs_ << reg_list_[value_reg] << std::endl;
            break;
        default:
            std::cerr << "Error: cannot read through this pointer!" << std::endl;
            std::exit(1);
    }

    return value_reg;
}

void X86Codegen::StoreDeref(reg_idx value_reg,
    reg_idx address_reg,
    PrimitiveType type)
{
    switch (PrimitiveSize(type))
    {
        case 1:
            ofs_ << "\tmovb\t" << breg_list_[value_reg] << ", ";
            break;
        case 4:
            ofs_ << "\tmovl\t" << dreg_list_[value_reg] << ", ";
            break;
        case 8:
            ofs_ << "\tmovq\t" << reg_list_[value_reg] << ", ";
            break;
        default:
            std::cerr << "Error: cannot write through this pointer!";
            std::cerr << std::endl;
            std::exit(1);
    }

    ofs_ << "(" << reg_list_[address_reg] << ")" << std::endl;
}

/*
 * The widths of the language's types on the x86-64 machine. An int is four bytes,
 * a long eight and a pointer as wide as one, which is what the machine
 * this compiler runs on has as well.
 */
const TypeSizes& TargetSizes()
{
    static const TypeSizes kSizes{1, 4, 8, 8};

    return kSizes;
}

/* The target writes the assembly of on the x86-64 machine. */
std::string TargetName()
{
    return "x86_64";
}

std::unique_ptr<AsmCodegen> MakeCodegen(const std::string& output_file)
{
    return std::make_unique<X86Codegen>(output_file);
}

}   /* namespace nuocc */
