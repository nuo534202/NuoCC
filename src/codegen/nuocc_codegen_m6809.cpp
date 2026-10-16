#include "codegen/nuocc_codegen_m6809.hpp"

#include <stdlib.h>

#include <iostream>
#include <string_view>
#include <unordered_map>

#include "codegen/nuocc_peephole.hpp"
#include "utils/nuocc_command.hpp"
#include "utils/nuocc_runtime.hpp"
#include "utils/nuocc_type_check.hpp"

namespace nuocc
{

namespace
{

/*
 * The branches which hold when each comparison does. The one taken when
 * the comparison does not hold is a long branch, since the label it goes
 * to is as far away as the whole condition is long.
 */
struct ComparisonInstr
{
    /* Taken when the comparison holds and the high halves of two long
     * values already say so. */
    std::string_view holds;
    /* Taken when it does not hold. */
    std::string_view fails;
};

const std::unordered_map<NodeTag, ComparisonInstr> kComparisonInstr = {
    {T_EQ, {"",     "lbne"}},
    {T_NE, {"lbne", "lbeq"}},
    {T_LT, {"lblt", "lbge"}},
    {T_GT, {"lbgt", "lble"}},
    {T_LE, {"lblt", "lbgt"}},
    {T_GE, {"lbgt", "lblt"}}
};

/* The comparison which holds where an ordered one does not, which is what
 * settles a long whose high halves are not the same: two values are less
 * than each other when the high halves are. */
const std::unordered_map<NodeTag, NodeTag> kInverseCompare = {
    {T_LT, T_GT},
    {T_GT, T_LT},
    {T_LE, T_GE},
    {T_GE, T_LE}
};

/*
 * The branches which hold when each comparison does, for the low halves of
 * two values whose high halves are the same. What is compared then is the
 * bottom half of a number rather than a number of its own: two numbers
 * whose high halves are the same are in the order of their low halves read
 * as values which have no sign, however negative either number is.
 */
const std::unordered_map<NodeTag, ComparisonInstr> kLowComparisonInstr = {
    {T_EQ, {"",     "lbne"}},
    {T_NE, {"lbne", "lbeq"}},
    {T_LT, {"lblo", "lbhs"}},
    {T_GT, {"lbhi", "lbls"}},
    {T_LE, {"lbls", "lbhi"}},
    {T_GE, {"lbhs", "lblo"}}
};

/* The two halves of a long: the high half comes first, since the machine
 * is big endian. */
constexpr int32 kHighHalf = 0;
constexpr int32 kLowHalf = 2;

/* How much room one value takes in the frame, so that a register's place
 * holds whatever the walk puts in it. */
constexpr int32 kPlaceSize = 4;
constexpr int32 kPlaceCount = kRegSize;
constexpr int32 kSpillArea = kSpillSlots * kPlaceSize;

/* Where the runtime keeps the value a function answers with, and the value
 * a multiplication is given when it is not known while the program is
 * being compiled. Both are named by the runtime. */
constexpr char kReturnName[] = "__ret";
constexpr char kScaleName[] = "__scale";

/* The routines of the runtime which work out what the machine has no
 * instruction for. */
constexpr char kMul16[] = "__mul16";
constexpr char kMul32[] = "__mul32";
constexpr char kDiv16[] = "__div16";
constexpr char kDiv32[] = "__div32";
constexpr char kRem16[] = "__rem16";
constexpr char kRem32[] = "__rem32";
constexpr char kNeg32[] = "__neg32";

/*
 * The runtime every 6809 program is built with: the path is fixed when the
 * compiler is built, the way the path of the C runtime is.
 */
#ifndef NUOCC_6809_RUNTIME
#define NUOCC_6809_RUNTIME "lib/6809/runtime.s"
#endif

constexpr char kRuntimeName[] = NUOCC_6809_RUNTIME;

}   /* namespace */

M6809Codegen::M6809Codegen(const std::string& output_file)
    : AsmCodegen(output_file)
{
}

int32 M6809Codegen::SizeOf(PrimitiveType type)
{
    return PrimitiveSize(type);
}

/*
 * The frame of a function is addressed through the U register, which is
 * given the frame while the body of the function runs. The stack pointer
 * moves as arguments are put on the stack for the calls the body makes, so
 * a place in the frame cannot be named by it.
 */
std::string M6809Codegen::Frame(int32 offset)
{
    return std::to_string(offset) + ",u";
}

std::string M6809Codegen::Slot(reg_idx reg, int32 byte) const
{
    return Frame(slot_base_ + static_cast<int32>(reg) * kPlaceSize + byte);
}

std::string M6809Codegen::SpillSlot(int32 slot, int32 byte) const
{
    const int32 base = slot_base_ + kPlaceCount * kPlaceSize;

    return Frame(base + slot * kPlaceSize + byte);
}

std::string M6809Codegen::StringName(label_idx label)
{
    return "__str" + std::to_string(label);
}

std::string M6809Codegen::Label(label_idx label) const
{
    return "__L" + UnitName() + "_" + std::to_string(label);
}

std::string M6809Codegen::Name(const Symbol& symbol) const
{
    if (!symbol.is_static)
        return symbol.name;

    return "__" + UnitName() + "_" + symbol.name;
}

/*
 * The instructions which move a value between a place and a register. A
 * value of four bytes is moved a half at a time, the high one first, so
 * that what the place holds is the same whichever half is being worked on.
 */
void M6809Codegen::LoadNamed(reg_idx reg, const std::string& name, int32 size)
{
    if (size <= 2)
    {
        /* A char is read on its own and the zeroes above it are put in
         * here, since a char has no sign. */
        if (size == 1)
        {
            ofs_ << "\tclra" << std::endl;
            ofs_ << "\tldb\t" << name << std::endl;
        }
        else
            ofs_ << "\tldd\t" << name << std::endl;

        ofs_ << "\tstd\t" << Slot(reg, kLowHalf) << std::endl;
        return;
    }

    ofs_ << "\tldd\t" << name << std::endl;
    ofs_ << "\tstd\t" << Slot(reg, kHighHalf) << std::endl;
    ofs_ << "\tldd\t" << name << "+2" << std::endl;
    ofs_ << "\tstd\t" << Slot(reg, kLowHalf) << std::endl;
}

void M6809Codegen::LoadFrame(reg_idx reg, int32 offset, int32 size)
{
    if (size <= 2)
    {
        if (size == 1)
        {
            ofs_ << "\tclra" << std::endl;
            ofs_ << "\tldb\t" << Frame(offset) << std::endl;
        }
        else
            ofs_ << "\tldd\t" << Frame(offset) << std::endl;

        ofs_ << "\tstd\t" << Slot(reg, kLowHalf) << std::endl;
        return;
    }

    ofs_ << "\tldd\t" << Frame(offset) << std::endl;
    ofs_ << "\tstd\t" << Slot(reg, kHighHalf) << std::endl;
    ofs_ << "\tldd\t" << Frame(offset + 2) << std::endl;
    ofs_ << "\tstd\t" << Slot(reg, kLowHalf) << std::endl;
}

void M6809Codegen::StoreNamed(reg_idx reg, const std::string& name, int32 size)
{
    if (size == 1)
    {
        /* A char takes one byte, and what stands next to it in memory
         * belongs to something else. */
        ofs_ << "\tldb\t" << Slot(reg, kPlaceSize - 1) << std::endl;
        ofs_ << "\tstb\t" << name << std::endl;
        return;
    }

    if (size == 2)
    {
        ofs_ << "\tldd\t" << Slot(reg, kLowHalf) << std::endl;
        ofs_ << "\tstd\t" << name << std::endl;
        return;
    }

    ofs_ << "\tldd\t" << Slot(reg, kHighHalf) << std::endl;
    ofs_ << "\tstd\t" << name << std::endl;
    ofs_ << "\tldd\t" << Slot(reg, kLowHalf) << std::endl;
    ofs_ << "\tstd\t" << name << "+2" << std::endl;
}

void M6809Codegen::StoreFrame(reg_idx reg, int32 offset, int32 size)
{
    if (size == 1)
    {
        /* A char takes one byte, and what stands next to it in the frame
         * belongs to something else. */
        ofs_ << "\tldb\t" << Slot(reg, kPlaceSize - 1) << std::endl;
        ofs_ << "\tstb\t" << Frame(offset) << std::endl;
        return;
    }

    if (size == 2)
    {
        ofs_ << "\tldd\t" << Slot(reg, kLowHalf) << std::endl;
        ofs_ << "\tstd\t" << Frame(offset) << std::endl;
        return;
    }

    ofs_ << "\tldd\t" << Slot(reg, kHighHalf) << std::endl;
    ofs_ << "\tstd\t" << Frame(offset) << std::endl;
    ofs_ << "\tldd\t" << Slot(reg, kLowHalf) << std::endl;
    ofs_ << "\tstd\t" << Frame(offset + 2) << std::endl;
}

/* The address of the value is already in X, which is left alone. */
void M6809Codegen::LoadThrough(reg_idx reg, int32 size)
{
    if (size <= 2)
    {
        if (size == 1)
        {
            ofs_ << "\tclra" << std::endl;
            ofs_ << "\tldb\t,x" << std::endl;
        }
        else
            ofs_ << "\tldd\t,x" << std::endl;

        ofs_ << "\tstd\t" << Slot(reg, kLowHalf) << std::endl;
        return;
    }

    ofs_ << "\tldd\t,x" << std::endl;
    ofs_ << "\tstd\t" << Slot(reg, kHighHalf) << std::endl;
    ofs_ << "\tldd\t2,x" << std::endl;
    ofs_ << "\tstd\t" << Slot(reg, kLowHalf) << std::endl;
}

void M6809Codegen::StoreThrough(reg_idx reg, int32 size)
{
    if (size == 1)
    {
        /* What stands next to a char in memory belongs to something
         * else. */
        ofs_ << "\tldb\t" << Slot(reg, kPlaceSize - 1) << std::endl;
        ofs_ << "\tstb\t,x" << std::endl;
        return;
    }

    if (size == 2)
    {
        ofs_ << "\tldd\t" << Slot(reg, kLowHalf) << std::endl;
        ofs_ << "\tstd\t,x" << std::endl;
        return;
    }

    ofs_ << "\tldd\t" << Slot(reg, kHighHalf) << std::endl;
    ofs_ << "\tstd\t,x" << std::endl;
    ofs_ << "\tldd\t" << Slot(reg, kLowHalf) << std::endl;
    ofs_ << "\tstd\t2,x" << std::endl;
}

void M6809Codegen::CopyValue(reg_idx dest, reg_idx src, int32 size)
{
    if (size <= 2)
    {
        ofs_ << "\tldd\t" << Slot(src, kLowHalf) << std::endl;
        ofs_ << "\tstd\t" << Slot(dest, kLowHalf) << std::endl;
        return;
    }

    ofs_ << "\tldd\t" << Slot(src, kHighHalf) << std::endl;
    ofs_ << "\tstd\t" << Slot(dest, kHighHalf) << std::endl;
    ofs_ << "\tldd\t" << Slot(src, kLowHalf) << std::endl;
    ofs_ << "\tstd\t" << Slot(dest, kLowHalf) << std::endl;
}

void M6809Codegen::EmitPreamble()
{
    /*
     * Nothing is written in front of the program: the runtime the program
     * is built with is what begins it, and the code of the program follows
     * it.
     */
}

/*
 * The storage of the string literals the program holds stands after the
 * last of its code, so that the program never runs into it.
 */
void M6809Codegen::EmitEndOfProgram()
{
    for (const std::pair<label_idx, std::string>& held : strings_)
    {
        ofs_ << StringName(held.first) << ":" << std::endl;
        ofs_ << "\t.byte\t";

        for (const char letter : held.second)
            ofs_ << static_cast<int>(static_cast<unsigned char>(letter))
                 << ",";

        ofs_ << "0" << std::endl;
    }
}

void M6809Codegen::EmitFunctionPreamble(
    const Symbol& symbol,
    int32 local_size,
    const std::vector<Symbol>& parameters)
{
    /* The places of the four registers follow the locals of the function,
     * and the places a value is set aside in follow those. A frame is
     * lined up to a whole number of two byte values, which is what the
     * machine reads and writes at a time. */
    slot_base_ = (local_size + 1) / 2 * 2;
    frame_size_ = slot_base_ + kPlaceCount * kPlaceSize + kSpillArea;

    ofs_ << Name(symbol) << ":" << std::endl;

    /*
     * The frame of the function being left is remembered, and the frame of
     * this one is taken below it. U is pointed at this frame, and is what
     * everything in the body is named by: the stack pointer moves as
     * arguments are put on the stack for the calls the body makes, and a
     * place in the frame cannot be named by something which moves.
     */
    ofs_ << "\tpshs\tu" << std::endl;
    ofs_ << "\tleas\t-" << frame_size_ << ",s" << std::endl;
    ofs_ << "\tleau\t,s" << std::endl;

    /*
     * Each argument arrives on the stack above the frame which was
     * remembered and the return address, and is kept in the frame of this
     * function, where the body reads it as the local variable it is. They
     * were pushed from the last one to the first, so the first one is
     * nearest the return address.
     */
    int32 incoming = frame_size_ + 4;

    for (const Symbol& parameter : parameters)
    {
        const int32 size = SizeOf(parameter.type);

        if (size == 1)
        {
            ofs_ << "\tldb\t" << Frame(incoming + 1) << std::endl;
            ofs_ << "\tstb\t" << Frame(parameter.stack_offset) << std::endl;
        }
        else if (size == 2)
        {
            ofs_ << "\tldd\t" << Frame(incoming) << std::endl;
            ofs_ << "\tstd\t" << Frame(parameter.stack_offset) << std::endl;
        }
        else
        {
            ofs_ << "\tldd\t" << Frame(incoming) << std::endl;
            ofs_ << "\tstd\t" << Frame(parameter.stack_offset) << std::endl;
            ofs_ << "\tldd\t" << Frame(incoming + 2) << std::endl;
            ofs_ << "\tstd\t" << Frame(parameter.stack_offset + 2);
            ofs_ << std::endl;
        }

        incoming += size <= 2 ? 2 : 4;
    }
}

void M6809Codegen::EmitFunctionPostamble()
{
    EmitLabel(function_end_label_);

    /* The frame is given back, the frame of the function which was left
     * is put back, and the return address is what leaves the function. */
    ofs_ << "\tleas\t" << frame_size_ << ",s" << std::endl;
    ofs_ << "\tpuls\tu" << std::endl;
    ofs_ << "\trts" << std::endl;
}

void M6809Codegen::EmitLabel(label_idx label)
{
    ofs_ << Label(label) << ":" << std::endl;
}

void M6809Codegen::EmitJump(label_idx label)
{
    ofs_ << "\tjmp\t" << Label(label) << std::endl;
}

reg_idx M6809Codegen::LoadInt(int32 value, PrimitiveType type)
{
    reg_idx reg = AllocRegister();

    if (SizeOf(type) <= 2)
    {
        ofs_ << "\tldd\t#" << (value & 0xffff) << std::endl;
        ofs_ << "\tstd\t" << Slot(reg, kLowHalf) << std::endl;

        return reg;
    }

    ofs_ << "\tldd\t#" << ((value >> 16) & 0xffff) << std::endl;
    ofs_ << "\tstd\t" << Slot(reg, kHighHalf) << std::endl;
    ofs_ << "\tldd\t#" << (value & 0xffff) << std::endl;
    ofs_ << "\tstd\t" << Slot(reg, kLowHalf) << std::endl;

    return reg;
}

reg_idx M6809Codegen::LoadSymbol(const Symbol& symbol)
{
    reg_idx reg = AllocRegister();
    const int32 size = SizeOf(symbol.type);

    if (symbol.storage == StorageClass::kLocal)
        LoadFrame(reg, symbol.stack_offset, size);
    else
        LoadNamed(reg, Name(symbol), size);

    return reg;
}

reg_idx M6809Codegen::StoreSymbol(const Symbol& symbol, reg_idx reg)
{
    const int32 size = SizeOf(symbol.type);

    if (symbol.storage == StorageClass::kLocal)
        StoreFrame(reg, symbol.stack_offset, size);
    else
        StoreNamed(reg, Name(symbol), size);

    return reg;
}

/*
 * The arithmetic the machine has an instruction for. What is added,
 * subtracted or worked on bit by bit is the low half of a long first, so
 * that what it carries or borrows is taken in by the high half.
 */
reg_idx M6809Codegen::Add(reg_idx reg1, reg_idx reg2, PrimitiveType type)
{
    ofs_ << "\tldd\t" << Slot(reg1, kLowHalf) << std::endl;
    ofs_ << "\taddd\t" << Slot(reg2, kLowHalf) << std::endl;
    ofs_ << "\tstd\t" << Slot(reg1, kLowHalf) << std::endl;

    if (SizeOf(type) > 2)
    {
        ofs_ << "\tldd\t" << Slot(reg1, kHighHalf) << std::endl;
        ofs_ << "\tadcb\t" << Slot(reg2, kHighHalf + 1) << std::endl;
        ofs_ << "\tadca\t" << Slot(reg2, kHighHalf) << std::endl;
        ofs_ << "\tstd\t" << Slot(reg1, kHighHalf) << std::endl;
    }

    FreeRegister(reg2);

    return reg1;
}

reg_idx M6809Codegen::Sub(reg_idx reg1, reg_idx reg2, PrimitiveType type)
{
    ofs_ << "\tldd\t" << Slot(reg1, kLowHalf) << std::endl;
    ofs_ << "\tsubd\t" << Slot(reg2, kLowHalf) << std::endl;
    ofs_ << "\tstd\t" << Slot(reg1, kLowHalf) << std::endl;

    if (SizeOf(type) > 2)
    {
        ofs_ << "\tldd\t" << Slot(reg1, kHighHalf) << std::endl;
        ofs_ << "\tsbcb\t" << Slot(reg2, kHighHalf + 1) << std::endl;
        ofs_ << "\tsbca\t" << Slot(reg2, kHighHalf) << std::endl;
        ofs_ << "\tstd\t" << Slot(reg1, kHighHalf) << std::endl;
    }

    FreeRegister(reg2);

    return reg1;
}

/* The machine works on two values at a time with the D accumulator, so
 * what is worked on bit by bit is read into it and written back. */
reg_idx M6809Codegen::And(reg_idx reg1, reg_idx reg2, PrimitiveType type)
{
    ofs_ << "\tldd\t" << Slot(reg1, kLowHalf) << std::endl;
    ofs_ << "\tanda\t" << Slot(reg2, kLowHalf) << std::endl;
    ofs_ << "\tandb\t" << Slot(reg2, kLowHalf + 1) << std::endl;
    ofs_ << "\tstd\t" << Slot(reg1, kLowHalf) << std::endl;

    if (SizeOf(type) > 2)
    {
        ofs_ << "\tldd\t" << Slot(reg1, kHighHalf) << std::endl;
        ofs_ << "\tanda\t" << Slot(reg2, kHighHalf) << std::endl;
        ofs_ << "\tandb\t" << Slot(reg2, kHighHalf + 1) << std::endl;
        ofs_ << "\tstd\t" << Slot(reg1, kHighHalf) << std::endl;
    }

    FreeRegister(reg2);

    return reg1;
}

reg_idx M6809Codegen::Or(reg_idx reg1, reg_idx reg2, PrimitiveType type)
{
    ofs_ << "\tldd\t" << Slot(reg1, kLowHalf) << std::endl;
    ofs_ << "\tora\t" << Slot(reg2, kLowHalf) << std::endl;
    ofs_ << "\torb\t" << Slot(reg2, kLowHalf + 1) << std::endl;
    ofs_ << "\tstd\t" << Slot(reg1, kLowHalf) << std::endl;

    if (SizeOf(type) > 2)
    {
        ofs_ << "\tldd\t" << Slot(reg1, kHighHalf) << std::endl;
        ofs_ << "\tora\t" << Slot(reg2, kHighHalf) << std::endl;
        ofs_ << "\torb\t" << Slot(reg2, kHighHalf + 1) << std::endl;
        ofs_ << "\tstd\t" << Slot(reg1, kHighHalf) << std::endl;
    }

    FreeRegister(reg2);

    return reg1;
}

reg_idx M6809Codegen::Xor(reg_idx reg1, reg_idx reg2, PrimitiveType type)
{
    ofs_ << "\tldd\t" << Slot(reg1, kLowHalf) << std::endl;
    ofs_ << "\teora\t" << Slot(reg2, kLowHalf) << std::endl;
    ofs_ << "\teorb\t" << Slot(reg2, kLowHalf + 1) << std::endl;
    ofs_ << "\tstd\t" << Slot(reg1, kLowHalf) << std::endl;

    if (SizeOf(type) > 2)
    {
        ofs_ << "\tldd\t" << Slot(reg1, kHighHalf) << std::endl;
        ofs_ << "\teora\t" << Slot(reg2, kHighHalf) << std::endl;
        ofs_ << "\teorb\t" << Slot(reg2, kHighHalf + 1) << std::endl;
        ofs_ << "\tstd\t" << Slot(reg1, kHighHalf) << std::endl;
    }

    FreeRegister(reg2);

    return reg1;
}

/*
 * What the machine has no instruction for is worked out by the runtime,
 * which is given the address of each of the two values in the frame and
 * leaves its answer where the first of them is.
 */
reg_idx M6809Codegen::EmitRuntimeBinary(reg_idx reg1,
    reg_idx reg2,
    PrimitiveType type,
    std::string_view helper32,
    std::string_view helper16)
{
    if (SizeOf(type) <= 2)
    {
        ofs_ << "\tleax\t" << Slot(reg1, kLowHalf) << std::endl;
        ofs_ << "\tleay\t" << Slot(reg2, kLowHalf) << std::endl;
        ofs_ << "\tjsr\t" << helper16 << std::endl;
    }
    else
    {
        ofs_ << "\tleax\t" << Slot(reg1, kHighHalf) << std::endl;
        ofs_ << "\tleay\t" << Slot(reg2, kHighHalf) << std::endl;
        ofs_ << "\tjsr\t" << helper32 << std::endl;
    }

    FreeRegister(reg2);

    return reg1;
}

reg_idx M6809Codegen::Mul(reg_idx reg1, reg_idx reg2, PrimitiveType type)
{
    return EmitRuntimeBinary(reg1, reg2, type, kMul32, kMul16);
}

reg_idx M6809Codegen::Div(reg_idx reg1, reg_idx reg2, PrimitiveType type)
{
    return EmitRuntimeBinary(reg1, reg2, type, kDiv32, kDiv16);
}

reg_idx M6809Codegen::Mod(reg_idx reg1, reg_idx reg2, PrimitiveType type)
{
    return EmitRuntimeBinary(reg1, reg2, type, kRem32, kRem16);
}

/*
 * The machine shifts a value along by one bit at a time, so a shift by a
 * number of bits which is only known when the program runs is a loop. The
 * low byte of the value shifted by is how many times it goes round.
 */
reg_idx M6809Codegen::EmitShift(reg_idx reg1, reg_idx reg2, int32 size, bool left)
{
    const label_idx loop = NewLabel();
    const label_idx done = NewLabel();

    ofs_ << "\tldb\t" << Slot(reg2, kPlaceSize - 1) << std::endl;
    ofs_ << "\tlbeq\t" << Label(done) << std::endl;
    EmitLabel(loop);

    if (left)
    {
        for (int32 byte = kPlaceSize - 1; byte >= kPlaceSize - size; byte--)
            ofs_ << "\t" << (byte == kPlaceSize - 1 ? "lsl" : "rol")
                 << "\t" << Slot(reg1, byte) << std::endl;
    }
    else
    {
        for (int32 byte = kPlaceSize - size; byte < kPlaceSize; byte++)
            ofs_ << "\t" << (byte == kPlaceSize - size ? "lsr" : "ror")
                 << "\t" << Slot(reg1, byte) << std::endl;
    }

    ofs_ << "\tdecb" << std::endl;
    ofs_ << "\tlbne\t" << Label(loop) << std::endl;
    EmitLabel(done);

    FreeRegister(reg2);

    return reg1;
}

reg_idx M6809Codegen::ShiftLeft(reg_idx reg1,
    reg_idx reg2,
    PrimitiveType type)
{
    return EmitShift(reg1, reg2, SizeOf(type), true);
}

reg_idx M6809Codegen::ShiftRight(reg_idx reg1,
    reg_idx reg2,
    PrimitiveType type)
{
    return EmitShift(reg1, reg2, SizeOf(type), false);
}

void M6809Codegen::EmitFillHighHalf(reg_idx reg, bool from_sign)
{
    if (!from_sign)
    {
        ofs_ << "\tldd\t#0" << std::endl;
        ofs_ << "\tstd\t" << Slot(reg, kHighHalf) << std::endl;
        return;
    }

    /* The bit which says which half of the number the value is is the top
     * bit of it, so the bytes above it are that bit over and over. */
    const label_idx positive = NewLabel();
    const label_idx done = NewLabel();

    ofs_ << "\tldd\t" << Slot(reg, kLowHalf) << std::endl;
    ofs_ << "\tlbpl\t" << Label(positive) << std::endl;
    ofs_ << "\tldd\t#65535" << std::endl;
    ofs_ << "\tstd\t" << Slot(reg, kHighHalf) << std::endl;
    EmitJump(done);
    EmitLabel(positive);
    ofs_ << "\tldd\t#0" << std::endl;
    ofs_ << "\tstd\t" << Slot(reg, kHighHalf) << std::endl;
    EmitLabel(done);
}

/*
 * Nothing has to be done to widen a value which is held as the whole of
 * it, but a value which is narrower than the type it is widened to has to
 * have the bytes above it filled in: a char has none of the sign it has
 * not got, and an int is filled with a copy of the bit which says which
 * half of the number it is.
 */
reg_idx M6809Codegen::Widen(reg_idx reg,
    PrimitiveType old_type,
    PrimitiveType new_type)
{
    const int32 old_size = SizeOf(old_type);
    const int32 new_size = SizeOf(new_type);

    if (new_size <= old_size || new_size <= 2)
        return reg;

    EmitFillHighHalf(reg, old_size > 1);

    return reg;
}

/*
 * A value which is set aside is copied out of the place its register
 * keeps it in, since the register goes on to hold another value.
 */
void M6809Codegen::SpillRegister(reg_idx reg, int32 slot)
{
    ofs_ << "\tldd\t" << Slot(reg, kHighHalf) << std::endl;
    ofs_ << "\tstd\t" << SpillSlot(slot, kHighHalf) << std::endl;
    ofs_ << "\tldd\t" << Slot(reg, kLowHalf) << std::endl;
    ofs_ << "\tstd\t" << SpillSlot(slot, kLowHalf) << std::endl;
}

void M6809Codegen::ReloadRegister(reg_idx reg, int32 slot)
{
    ofs_ << "\tldd\t" << SpillSlot(slot, kHighHalf) << std::endl;
    ofs_ << "\tstd\t" << Slot(reg, kHighHalf) << std::endl;
    ofs_ << "\tldd\t" << SpillSlot(slot, kLowHalf) << std::endl;
    ofs_ << "\tstd\t" << Slot(reg, kLowHalf) << std::endl;
}

/*
 * Give a value the type it was cast to. A char keeps the low byte of what
 * it is given and has zeroes above it, since it has no sign; an int and a
 * pointer keep the low two bytes, which is where the low half of a long
 * already is; and a long takes the value it is given with the bytes above
 * it filled in.
 */
reg_idx M6809Codegen::Cast(reg_idx reg,
    PrimitiveType old_type,
    PrimitiveType new_type)
{
    const int32 size = SizeOf(new_type);

    if (size == 1)
    {
        ofs_ << "\tldb\t" << Slot(reg, kPlaceSize - 1) << std::endl;
        ofs_ << "\tclra" << std::endl;
        ofs_ << "\tstd\t" << Slot(reg, kLowHalf) << std::endl;

        return reg;
    }

    if (size > 2)
        EmitFillHighHalf(reg, SizeOf(old_type) > 1);

    return reg;
}

/*
 * A place which is a stride of a value along is worked out by adding the
 * stride to the value, which is what a long or an int of it is the same
 * for either way.
 */
reg_idx M6809Codegen::Scale(reg_idx reg, int32 scale, PrimitiveType type)
{
    if (scale == 1)
        return reg;

    const int32 size = SizeOf(type);

    if (scale == 2 || scale == 4 || scale == 8)
    {
        /* A value is strided along by being shifted, and a stride which
         * is a power of two is that many shifts. */
        const int32 shifts = scale == 2 ? 1 : scale == 4 ? 2 : 3;

        for (int32 k = 0; k < shifts; k++)
            for (int32 byte = kPlaceSize - 1; byte >= kPlaceSize - size;
                 byte--)
                ofs_ << "\t" << (byte == kPlaceSize - 1 ? "lsl" : "rol")
                     << "\t" << Slot(reg, byte) << std::endl;

        return reg;
    }

    /*
     * A stride which is not a power of two, which is the room a composite
     * type takes, is worked out by the runtime: it is given the stride in
     * a place of its own, since a multiplication of two values has one of
     * them at the address of the other.
     */
    ofs_ << "\tldd\t#" << (scale & 0xffff) << std::endl;
    ofs_ << "\tstd\t" << kScaleName << "+2" << std::endl;
    ofs_ << "\tldd\t#0" << std::endl;
    ofs_ << "\tstd\t" << kScaleName << std::endl;

    if (size <= 2)
    {
        ofs_ << "\tleax\t" << Slot(reg, kLowHalf) << std::endl;
        ofs_ << "\tldy\t#" << kScaleName << "+2" << std::endl;
        ofs_ << "\tjsr\t" << kMul16 << std::endl;
    }
    else
    {
        ofs_ << "\tleax\t" << Slot(reg, kHighHalf) << std::endl;
        ofs_ << "\tldy\t#" << kScaleName << std::endl;
        ofs_ << "\tjsr\t" << kMul32 << std::endl;
    }

    return reg;
}

void M6809Codegen::Move(reg_idx dest, reg_idx src, PrimitiveType type)
{
    CopyValue(dest, src, SizeOf(type));
}

reg_idx M6809Codegen::Negate(reg_idx reg, PrimitiveType type)
{
    if (SizeOf(type) <= 2)
    {
        ofs_ << "\tldd\t" << Slot(reg, kLowHalf) << std::endl;
        ofs_ << "\tnega" << std::endl;
        ofs_ << "\tnegb" << std::endl;
        ofs_ << "\tsbca\t#0" << std::endl;
        ofs_ << "\tstd\t" << Slot(reg, kLowHalf) << std::endl;

        return reg;
    }

    ofs_ << "\tleax\t" << Slot(reg, kHighHalf) << std::endl;
    ofs_ << "\tjsr\t" << kNeg32 << std::endl;

    return reg;
}

reg_idx M6809Codegen::Invert(reg_idx reg, PrimitiveType type)
{
    const int32 size = SizeOf(type) <= 2 ? 2 : 4;

    for (int32 byte = kPlaceSize - size; byte < kPlaceSize; byte++)
        ofs_ << "\tcom\t" << Slot(reg, byte) << std::endl;

    return reg;
}

/*
 * The answer to either of these is a whole number: one when the value is
 * nothing and nothing when it is not, or the other way round.
 */
void M6809Codegen::EmitTestZero(reg_idx reg, int32 size, label_idx label)
{
    if (size <= 2)
    {
        ofs_ << "\tldd\t" << Slot(reg, kLowHalf) << std::endl;
        ofs_ << "\tlbeq\t" << Label(label) << std::endl;
        return;
    }

    /* A long is nothing only when both of its halves are. */
    const label_idx done = NewLabel();

    ofs_ << "\tldd\t" << Slot(reg, kHighHalf) << std::endl;
    ofs_ << "\tlbne\t" << Label(done) << std::endl;
    ofs_ << "\tldd\t" << Slot(reg, kLowHalf) << std::endl;
    ofs_ << "\tlbeq\t" << Label(label) << std::endl;
    EmitLabel(done);
}

reg_idx M6809Codegen::LogNot(reg_idx reg, PrimitiveType type)
{
    const int32 size = SizeOf(type) <= 2 ? 2 : 4;
    const label_idx nothing = NewLabel();
    const label_idx done = NewLabel();

    EmitTestZero(reg, size, nothing);

    ofs_ << "\tldd\t#0" << std::endl;
    ofs_ << "\tstd\t" << Slot(reg, kLowHalf) << std::endl;
    EmitJump(done);
    EmitLabel(nothing);
    ofs_ << "\tldd\t#1" << std::endl;
    ofs_ << "\tstd\t" << Slot(reg, kLowHalf) << std::endl;
    EmitLabel(done);

    return reg;
}

reg_idx M6809Codegen::ToBool(reg_idx reg, PrimitiveType type)
{
    const int32 size = SizeOf(type) <= 2 ? 2 : 4;
    const label_idx nothing = NewLabel();
    const label_idx done = NewLabel();

    EmitTestZero(reg, size, nothing);

    ofs_ << "\tldd\t#1" << std::endl;
    ofs_ << "\tstd\t" << Slot(reg, kLowHalf) << std::endl;
    EmitJump(done);
    EmitLabel(nothing);
    ofs_ << "\tldd\t#0" << std::endl;
    ofs_ << "\tstd\t" << Slot(reg, kLowHalf) << std::endl;
    EmitLabel(done);

    return reg;
}

/* Add a number which the compiler knows to a value in a register. */
void M6809Codegen::EmitAddConstant(reg_idx reg, int64 value, int32 size)
{
    if (size <= 2)
    {
        ofs_ << "\tldd\t" << Slot(reg, kLowHalf) << std::endl;
        ofs_ << "\taddd\t#" << (value & 0xffff) << std::endl;
        ofs_ << "\tstd\t" << Slot(reg, kLowHalf) << std::endl;
        return;
    }

    ofs_ << "\tldd\t" << Slot(reg, kLowHalf) << std::endl;
    ofs_ << "\taddd\t#" << (value & 0xffff) << std::endl;
    ofs_ << "\tstd\t" << Slot(reg, kLowHalf) << std::endl;
    ofs_ << "\tldd\t" << Slot(reg, kHighHalf) << std::endl;
    ofs_ << "\tadcb\t#" << ((value >> 8) & 0xff) << std::endl;
    ofs_ << "\tadca\t#" << ((value >> 16) & 0xff) << std::endl;
    ofs_ << "\tstd\t" << Slot(reg, kHighHalf) << std::endl;
}

reg_idx M6809Codegen::IncDec(const Symbol& symbol, int32 delta, bool post)
{
    reg_idx old_reg = LoadSymbol(symbol);
    const int32 size = SizeOf(symbol.type);

    reg_idx new_reg = AllocRegister();
    CopyValue(new_reg, old_reg, size);
    EmitAddConstant(new_reg, delta, size);

    StoreSymbol(symbol, new_reg);

    if (post)
    {
        FreeRegister(new_reg);
        return old_reg;
    }

    FreeRegister(old_reg);

    return new_reg;
}

/*
 * Compare the two values and jump to the label when the comparison does
 * not hold. A value of four bytes is settled by its high half unless the
 * two high halves are the same, which is when the low ones are asked.
 */
void M6809Codegen::EmitCompareJump(NodeTag op_type,
    reg_idx reg1,
    reg_idx reg2,
    int32 size,
    label_idx label)
{
    auto it = kComparisonInstr.find(op_type);

    if (it == kComparisonInstr.end())
    {
        std::cerr << "Error: token " << op_type;
        std::cerr << " is not a comparison!" << std::endl;
        std::exit(1);
    }

    /* A value of two bytes is a number of its own, so one comparison of
     * the two of them settles it. */
    if (size <= 2)
    {
        ofs_ << "\tldd\t" << Slot(reg1, kLowHalf) << std::endl;
        ofs_ << "\tcmpd\t" << Slot(reg2, kLowHalf) << std::endl;
        ofs_ << "\t" << it->second.fails << "\t" << Label(label) << std::endl;
        return;
    }

    /* A value of four bytes is settled by its high halves, which is the
     * half holding the sign: the two of them are compared as numbers which
     * have one. */
    ofs_ << "\tldd\t" << Slot(reg1, kHighHalf) << std::endl;
    ofs_ << "\tcmpd\t" << Slot(reg2, kHighHalf) << std::endl;

    /* Two values are the same only when both of their halves are, so a
     * difference in the high ones is the whole answer. */
    if (op_type == T_EQ)
    {
        ofs_ << "\t" << it->second.fails << "\t" << Label(label) << std::endl;
        ofs_ << "\tldd\t" << Slot(reg1, kLowHalf) << std::endl;
        ofs_ << "\tcmpd\t" << Slot(reg2, kLowHalf) << std::endl;
        ofs_ << "\t" << it->second.fails << "\t" << Label(label) << std::endl;
        return;
    }

    if (op_type == T_NE)
    {
        /* Different high halves are the values being different, so the
         * comparison holds and there is nothing left to ask. */
        const label_idx done = NewLabel();

        ofs_ << "\t" << it->second.holds << "\t" << Label(done) << std::endl;
        ofs_ << "\tldd\t" << Slot(reg1, kLowHalf) << std::endl;
        ofs_ << "\tcmpd\t" << Slot(reg2, kLowHalf) << std::endl;
        ofs_ << "\t" << it->second.fails << "\t" << Label(label) << std::endl;
        EmitLabel(done);
        return;
    }

    /*
     * An ordered comparison is settled on its own by a high half which is
     * greater or less than the other, whichever way the comparison runs.
     */
    const label_idx holds = NewLabel();

    ofs_ << "\t" << it->second.holds << "\t" << Label(holds) << std::endl;
    ofs_ << "\t" << kComparisonInstr.at(
        kInverseCompare.at(op_type)).holds << "\t" << Label(label);
    ofs_ << std::endl;

    /*
     * The two high halves are the same, so the low ones settle it. What is
     * compared then is the bottom half of a number rather than a number of
     * its own: two numbers whose high halves are the same stand in the
     * order of their low halves read as values which have no sign.
     */
    ofs_ << "\tldd\t" << Slot(reg1, kLowHalf) << std::endl;
    ofs_ << "\tcmpd\t" << Slot(reg2, kLowHalf) << std::endl;
    ofs_ << "\t" << kLowComparisonInstr.at(op_type).fails << "\t";
    ofs_ << Label(label) << std::endl;

    EmitLabel(holds);
}

reg_idx M6809Codegen::CompareAndSet(NodeTag op_type,
    reg_idx reg1,
    reg_idx reg2,
    PrimitiveType type)
{
    const label_idx fails = NewLabel();
    const label_idx done = NewLabel();

    EmitCompareJump(op_type, reg1, reg2, SizeOf(type), fails);

    /* The comparison holds, so the answer is one. */
    ofs_ << "\tldd\t#1" << std::endl;
    ofs_ << "\tstd\t" << Slot(reg1, kLowHalf) << std::endl;
    EmitJump(done);
    EmitLabel(fails);

    /* It does not, so the answer is nothing. */
    ofs_ << "\tldd\t#0" << std::endl;
    ofs_ << "\tstd\t" << Slot(reg1, kLowHalf) << std::endl;
    EmitLabel(done);

    FreeRegister(reg2);

    return reg1;
}

void M6809Codegen::CompareAndJump(NodeTag op_type,
    reg_idx reg1,
    reg_idx reg2,
    label_idx label,
    PrimitiveType type)
{
    EmitCompareJump(op_type, reg1, reg2, SizeOf(type), label);

    /* The comparison is a jump and answers with nothing, so both values
     * it was given are done with. Only those two are freed: what the rest
     * of the expression holds is not this comparison's to throw away. */
    FreeRegister(reg1);
    FreeRegister(reg2);
}

void M6809Codegen::JumpIfEqual(reg_idx reg1,
    reg_idx reg2,
    label_idx label,
    PrimitiveType type)
{
    if (SizeOf(type) <= 2)
    {
        ofs_ << "\tldd\t" << Slot(reg1, kLowHalf) << std::endl;
        ofs_ << "\tcmpd\t" << Slot(reg2, kLowHalf) << std::endl;
        ofs_ << "\tlbeq\t" << Label(label) << std::endl;
        return;
    }

    /* Both halves of the two values have to be the same. */
    const label_idx next = NewLabel();

    ofs_ << "\tldd\t" << Slot(reg1, kHighHalf) << std::endl;
    ofs_ << "\tcmpd\t" << Slot(reg2, kHighHalf) << std::endl;
    ofs_ << "\tlbne\t" << Label(next) << std::endl;
    ofs_ << "\tldd\t" << Slot(reg1, kLowHalf) << std::endl;
    ofs_ << "\tcmpd\t" << Slot(reg2, kLowHalf) << std::endl;
    ofs_ << "\tlbeq\t" << Label(label) << std::endl;
    EmitLabel(next);
}

void M6809Codegen::JumpIfZero(reg_idx reg, label_idx label)
{
    ofs_ << "\tldd\t" << Slot(reg, kLowHalf) << std::endl;
    ofs_ << "\tlbeq\t" << Label(label) << std::endl;
}

void M6809Codegen::JumpIfNonZero(reg_idx reg, label_idx label)
{
    ofs_ << "\tldd\t" << Slot(reg, kLowHalf) << std::endl;
    ofs_ << "\tlbne\t" << Label(label) << std::endl;
}

void M6809Codegen::PushArgument(reg_idx reg, PrimitiveType type)
{
    if (SizeOf(type) <= 2)
    {
        ofs_ << "\tldd\t" << Slot(reg, kLowHalf) << std::endl;
        ofs_ << "\tpshs\td" << std::endl;
    }
    else
    {
        /* The low half goes on first, so that the high one ends up in
         * front of it and the value reads the way it is held. */
        ofs_ << "\tldd\t" << Slot(reg, kLowHalf) << std::endl;
        ofs_ << "\tpshs\td" << std::endl;
        ofs_ << "\tldd\t" << Slot(reg, kHighHalf) << std::endl;
        ofs_ << "\tpshs\td" << std::endl;
    }

    arguments_.push_back(SizeOf(type) <= 2 ? 2 : 4);
}

reg_idx M6809Codegen::Call(const Symbol& symbol, int32 arg_count)
{
    ofs_ << "\tjsr\t" << Name(symbol) << std::endl;

    /*
     * A function which answers with nothing is not called for a value, but
     * the walk is given a register all the same and throws it away, so one
     * is handed out and left holding whatever it held.
     */
    reg_idx out_reg = AllocRegister();

    if (symbol.type != PrimitiveType::kVoid)
    {
        /* The value a function answers with is where its return left it,
         * and the whole of it is read into the register of the caller. */
        if (SizeOf(symbol.type) <= 2)
        {
            ofs_ << "\tldd\t" << kReturnName << std::endl;
            ofs_ << "\tstd\t" << Slot(out_reg, kLowHalf) << std::endl;
        }
        else
        {
            ofs_ << "\tldd\t" << kReturnName << std::endl;
            ofs_ << "\tstd\t" << Slot(out_reg, kHighHalf) << std::endl;
            ofs_ << "\tldd\t" << kReturnName << "+2" << std::endl;
            ofs_ << "\tstd\t" << Slot(out_reg, kLowHalf) << std::endl;
        }
    }

    /* The arguments are taken off the stack, which leaves it as it was
     * before the call was made. */
    const std::size_t first = arguments_.size() - arg_count;
    int32 taken = 0;

    for (int32 k = 0; k < arg_count; k++)
        taken += arguments_[first + k];

    if (taken != 0)
        ofs_ << "\tleas\t" << taken << ",s" << std::endl;

    arguments_.resize(first);

    return out_reg;
}

void M6809Codegen::Return(reg_idx reg)
{
    const int32 size = SizeOf(function_return_type_) <= 2 ? 2 : 4;

    StoreNamed(reg, kReturnName, size);

    EmitJump(function_end_label_);
}

void M6809Codegen::PrintInt(reg_idx reg)
{
    /* The value of a print statement is left in D, which is where the
     * routine the runtime keeps for it reads it. */
    ofs_ << "\tldd\t" << Slot(reg, kLowHalf) << std::endl;
    ofs_ << "\tjsr\t__printint" << std::endl;
}

reg_idx M6809Codegen::AddressOf(const Symbol& symbol)
{
    reg_idx reg = AllocRegister();

    if (symbol.storage == StorageClass::kLocal)
        ofs_ << "\tleax\t" << Frame(symbol.stack_offset) << std::endl;
    else
        ofs_ << "\tldx\t#" << Name(symbol) << std::endl;

    ofs_ << "\tstx\t" << Slot(reg, kLowHalf) << std::endl;

    return reg;
}

void M6809Codegen::AddOffset(reg_idx reg, int32 offset)
{
    if (offset == 0)
        return;

    ofs_ << "\tldd\t" << Slot(reg, kLowHalf) << std::endl;
    ofs_ << "\taddd\t#" << (offset & 0xffff) << std::endl;
    ofs_ << "\tstd\t" << Slot(reg, kLowHalf) << std::endl;
}

reg_idx M6809Codegen::LoadStrAddress(label_idx label)
{
    reg_idx reg = AllocRegister();

    ofs_ << "\tldx\t#" << StringName(label) << std::endl;
    ofs_ << "\tstx\t" << Slot(reg, kLowHalf) << std::endl;

    return reg;
}

reg_idx M6809Codegen::Deref(reg_idx reg, PrimitiveType pointer_type)
{
    ofs_ << "\tldx\t" << Slot(reg, kLowHalf) << std::endl;
    LoadThrough(reg, SizeOf(ValueAt(pointer_type)));

    return reg;
}

reg_idx M6809Codegen::LoadDeref(reg_idx address_reg, PrimitiveType type)
{
    reg_idx value_reg = AllocRegister();

    ofs_ << "\tldx\t" << Slot(address_reg, kLowHalf) << std::endl;
    LoadThrough(value_reg, SizeOf(type));

    return value_reg;
}

void M6809Codegen::StoreDeref(reg_idx value_reg,
    reg_idx address_reg,
    PrimitiveType type)
{
    ofs_ << "\tldx\t" << Slot(address_reg, kLowHalf) << std::endl;
    StoreThrough(value_reg, SizeOf(type));
}

/*
 * The storage of a global variable is a named place with the value it is
 * given after it. Nothing has to be lined up: the machine reads and writes
 * a value a byte at a time.
 */
void M6809Codegen::BeginGlobal(const Symbol& symbol, int32 /*alignment*/)
{
    ofs_ << Name(symbol) << ":" << std::endl;
}

/* The room a struct or a union takes, which starts out as zeroes. */
void M6809Codegen::EmitGlobalSpace(int32 size)
{
    ofs_ << "\trmb\t" << size << std::endl;
}

void M6809Codegen::EmitGlobalValue(int32 value, int32 size)
{
    switch (size)
    {
        case 1:
            ofs_ << "\t.byte\t" << (value & 0xff) << std::endl;
            return;
        case 2:
            ofs_ << "\t.word\t" << (value & 0xffff) << std::endl;
            return;
        case 4:
            ofs_ << "\t.word\t" << ((value >> 16) & 0xffff) << std::endl;
            ofs_ << "\t.word\t" << (value & 0xffff) << std::endl;
            return;
        default:
            break;
    }

    std::cerr << "Error: a global element of " << size;
    std::cerr << " bytes has no directive!" << std::endl;
    std::exit(1);
}

void M6809Codegen::EmitGlobalLabelRef(label_idx label)
{
    ofs_ << "\t.word\t" << StringName(label) << std::endl;
}

void M6809Codegen::EndGlobal()
{
}

/*
 * The storage of a string holds one byte per character and then the NUL
 * which ends it, under a name of its own which the code takes the address
 * of. It is gathered rather than written where it is found, since the code
 * of the program runs through everything which stands between one function
 * and the next.
 */
void M6809Codegen::GenStrStorage(label_idx label, const std::string& text)
{
    strings_.emplace_back(label, text);
}

std::string TargetName()
{
    return "m6809";
}

std::unique_ptr<AsmCodegen> MakeCodegen(const std::string& output_file)
{
    return std::make_unique<M6809Codegen>(output_file);
}

/*
 * The widths of the language's types on the 6809: a char is one byte, a
 * short, an int and a pointer two, since the machine reaches no more than
 * 64K of memory, and a long four. A short is as wide as an int here,
 * because the machine has no narrower number of its own.
 */
const TypeSizes& TargetSizes()
{
    static const TypeSizes kSizes{1, 2, 2, 4, 2};

    return kSizes;
}

/* The target writes the assembly of the 6809 itself, and its program is
 * the runtime and that assembly put together and assembled by lwasm. */
std::string CodeSuffix()
{
    return ".s";
}

std::string CodeTool()
{
    return "";
}

std::string CodeBuilder()
{
    return "lwasm";
}

/*
 * A 6809 program is the runtime and the code the compiler wrote, put
 * together and assembled. The runtime is what begins the program, so it
 * comes first, and lwasm reads all of it as one file: a program of more
 * than one source file is the code of each of them after the runtime, and
 * a name declared twice is caught by the assembler.
 */
std::string BuildCommand(const std::vector<std::string>& code,
    const std::string& program)
{
    std::string command = "cat " + Quote(kRuntimeName);

    for (const std::string& file : code)
        command += " " + Quote(file);

    return command + " | lwasm --format=raw -o " + Quote(program) + " -";
}

/*
 * The code of a program of this target is put through the peephole
 * optimiser once the whole of it has been written: what the optimiser
 * leaves is smaller code which does what the code the backend wrote does.
 */
bool OptimiseCode(const std::string& code)
{
    Peephole peephole(M6809PeepholeRules());

    return peephole.OptimiseFile(code);
}

}   /* namespace nuocc */
