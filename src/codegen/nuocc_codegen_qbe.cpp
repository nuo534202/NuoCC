#include "codegen/nuocc_codegen_qbe.hpp"

#include <stdlib.h>

#include <iostream>
#include <string_view>
#include <unordered_map>
#include <utility>

#include "utils/nuocc_runtime.hpp"
#include "utils/nuocc_type_check.hpp"

namespace nuocc
{

namespace
{

/* The QBE instructions which implement a comparison operator, and the ones
 * which implement the same comparison the other way round. The comparison
 * is written the other way round where a jump is taken when it does not
 * hold. */
struct ComparisonInstr
{
    /* Sets a value to 0 or 1, used inside an expression. */
    std::string_view set;
    /* Holds when the comparison does not, used as a condition. */
    std::string_view inverse;
};

const std::unordered_map<NodeTag, ComparisonInstr> kComparisonInstr = {
    {T_EQ, {"ceql", "cnel"}},
    {T_NE, {"cnel", "ceql"}},
    {T_LT, {"csltl", "csgel"}},
    {T_GT, {"csgtl", "cslel"}},
    {T_LE, {"cslel", "csgtl"}},
    {T_GE, {"csgel", "csltl"}}
};

/* The number of bytes a char, a short, an int and a long take. */
constexpr int32 kCharSize = 1;
constexpr int32 kShortSize = 2;
constexpr int32 kIntSize = 4;
constexpr int32 kLongSize = 8;

}   /* namespace */

QbeCodegen::QbeCodegen(const std::string& output_file)
    : AsmCodegen(output_file)
{
}

char QbeCodegen::SizeLetter(int32 size)
{
    switch (size)
    {
        case kCharSize:
            return 'b';
        case kShortSize:
            return 'h';
        case kIntSize:
            return 'w';
        case kLongSize:
            return 'l';
        default:
            break;
    }

    std::cerr << "Error: a value of " << size;
    std::cerr << " bytes has no width in QBE!" << std::endl;
    std::exit(1);
}

/*
 * The load which reads a value of a size: a char is read without a sign,
 * since it has none and a value of it is 0 to 255, and an int with one,
 * since reading it without would turn a negative value into a large
 * positive one.
 */
const char* QbeCodegen::LoadLetter(int32 size)
{
    switch (size)
    {
        case kCharSize:
            return "ub";
        case kShortSize:
            return "sh";
        case kIntSize:
            return "sw";
        case kLongSize:
            return "l";
        default:
            break;
    }

    std::cerr << "Error: a value of " << size;
    std::cerr << " bytes cannot be read!" << std::endl;
    std::exit(1);
}

std::string QbeCodegen::Label(label_idx label)
{
    return "@L" + std::to_string(label);
}

std::string QbeCodegen::Register(reg_idx reg)
{
    return "%r" + std::to_string(reg);
}

/*
 * A name which QBE has not seen before, for a value which is not a value of
 * one of the four registers: the answer to a comparison, an address, or a
 * value which is being held while the call it belongs to is made.
 */
std::string QbeCodegen::NewValue()
{
    return "%t" + std::to_string(++temporary_count_);
}

/*
 * The four registers are written with the operations of the language as
 * they stand: both of their values are read before anything is written
 * over them, so an operation which puts its result where one of them was
 * needs no name of its own.
 */
void QbeCodegen::EmitBinary(reg_idx reg,
    std::string_view op,
    const std::string& left,
    const std::string& right)
{
    ofs_ << "  " << Register(reg) << " =l " << op << " " << left << ", ";
    ofs_ << right << std::endl;
}

void QbeCodegen::EmitUnary(reg_idx reg, std::string_view op)
{
    ofs_ << "  " << Register(reg) << " =l " << op << " " << Register(reg);
    ofs_ << std::endl;
}

/*
 * The address of a place in the frame of the function. The frame is one
 * block of stack room, which QBE hands out in whole long words, so the
 * places the program worked out while it was parsed are the offsets into
 * it.
 */
std::string QbeCodegen::FrameAddress(int32 offset)
{
    if (offset == 0)
        return "%frame";

    const std::string address = NewValue();

    ofs_ << "  " << address << " =l add %frame, " << offset << std::endl;

    return address;
}

void QbeCodegen::StoreAt(const std::string& address,
    const std::string& value,
    int32 size)
{
    ofs_ << "  store" << SizeLetter(size) << " " << value << ", " << address;
    ofs_ << std::endl;
}

/*
 * The storage of the string literals a function held is written once that
 * function is over, so nothing waits for the end of the program.
 */
void QbeCodegen::EmitEndOfProgram()
{
}

void QbeCodegen::EmitPreamble()
{
    /*
     * Nothing is needed in front of the program: QBE writes the sections
     * and the directives of the machine it is aimed at itself.
     */
}

void QbeCodegen::WriteStringData(label_idx label, const std::string& text)
{
    ofs_ << "data $L" << label << " = { ";

    for (const char letter : text)
    {
        ofs_ << "b " << static_cast<int>(static_cast<unsigned char>(letter));
        ofs_ << ", ";
    }

    ofs_ << "b 0 }" << std::endl;
}

void QbeCodegen::WriteHeldStrings()
{
    for (const std::pair<label_idx, std::string>& held : held_strings_)
        WriteStringData(held.first, held.second);

    held_strings_.clear();
}

void QbeCodegen::EmitFunctionPreamble(
    const Symbol& symbol,
    int32 local_size,
    const std::vector<Symbol>& parameters)
{
    /* The values of the function before this one are done with. */
    temporary_count_ = 0;
    arguments_.clear();

    for (idx_t slot = 0; slot < kSpillSlots; slot++)
        set_aside_[slot].clear();

    /*
     * Every function of the language takes its arguments and answers with
     * a value of the whole width of a register, whatever the type of each
     * of them is, so one function is reached the same way by every caller.
     */
    if (!symbol.is_static)
        ofs_ << "export ";

    ofs_ << "function l $" << symbol.name << "(";

    for (idx_t k = 0; k < parameters.size(); k++)
    {
        if (k > 0)
            ofs_ << ", ";

        ofs_ << "l %arg" << k;
    }

    ofs_ << ") {" << std::endl;

    /* The instructions of a function stand in a block of their own. */
    EmitLabel(NewLabel());

    in_function_ = true;
    block_ended_ = false;

    /*
     * The room the locals of the function take, which was worked out while
     * the program was parsed, is one block of stack room: QBE hands it out
     * in whole long words, so it is lined up to one of those.
     */
    const int32 frame_size = (local_size + kLongSize - 1) / kLongSize;

    if (frame_size > 0)
        ofs_ << "  %frame =l alloc8 " << frame_size << std::endl;

    /*
     * Each argument arrives as a value of its own and is kept in the frame
     * of the function, where the body reads it as the local variable it is.
     */
    for (idx_t k = 0; k < parameters.size(); k++)
    {
        const Symbol& parameter = parameters[k];

        StoreAt(FrameAddress(parameter.stack_offset),
                "%arg" + std::to_string(k),
                PrimitiveSize(parameter.type));
    }
}

void QbeCodegen::EmitFunctionPostamble()
{
    EmitLabel(function_end_label_);

    /*
     * Where every return statement of the function jumps to, and where a
     * function which runs off the end of its body ends: one which answers
     * with a value answers with none of one rather than leaving the caller
     * with whatever happened to be around.
     */
    if (function_return_type_ == PrimitiveType::kVoid)
        ofs_ << "  ret" << std::endl;
    else
        ofs_ << "  ret 0" << std::endl;

    ofs_ << "}" << std::endl;

    in_function_ = false;

    /* The storage of the string literals the function held belongs with
     * the rest of the storage of the program. */
    WriteHeldStrings();
}

void QbeCodegen::EmitLabel(label_idx label)
{
    /* QBE takes the name of a label on a line of its own, with nothing
     * after it. */
    ofs_ << Label(label) << std::endl;

    /* The label begins a block, whatever came before it. */
    block_ended_ = false;
}

void QbeCodegen::EmitJump(label_idx label)
{
    ofs_ << "  jmp " << Label(label) << std::endl;

    block_ended_ = true;
}

/*
 * QBE writes a function as a series of blocks, each of which begins with a
 * label and ends in a jump, so no instruction may follow a jump in the same
 * block. A statement which jumped away therefore ends the block it stands
 * in, and what follows it, which is reached from somewhere else rather than
 * by carrying on, begins a block of its own here.
 */
void QbeCodegen::BeginStatement()
{
    if (!block_ended_)
        return;

    ofs_ << Label(NewLabel()) << std::endl;

    block_ended_ = false;
}

reg_idx QbeCodegen::LoadInt(int32 value, PrimitiveType /*type*/)
{
    reg_idx reg = AllocRegister();

    ofs_ << "  " << Register(reg) << " =l copy " << value << std::endl;

    return reg;
}

/*
 * Read a variable into a register. A local variable is read at the place it
 * takes in the frame of the function, and a global one at the name of its
 * storage, which is where the address of it is.
 */
reg_idx QbeCodegen::LoadSymbol(const Symbol& symbol)
{
    const std::string address =
        symbol.storage == StorageClass::kLocal
            ? FrameAddress(symbol.stack_offset)
            : "$" + symbol.name;

    reg_idx reg = AllocRegister();

    ofs_ << "  " << Register(reg) << " =l load";
    ofs_ << LoadLetter(PrimitiveSize(symbol.type));
    ofs_ << " " << address << std::endl;

    return reg;
}

reg_idx QbeCodegen::StoreSymbol(const Symbol& symbol, reg_idx reg)
{
    if (symbol.storage == StorageClass::kLocal)
        StoreAt(FrameAddress(symbol.stack_offset),
                Register(reg),
                PrimitiveSize(symbol.type));
    else
        StoreAt("$" + symbol.name, Register(reg), PrimitiveSize(symbol.type));

    return reg;
}

/*
 * The arithmetic operations. Each of them works out its result from the two
 * values the registers hold and gives it to the register the first of them
 * was in, exactly like Add() and the rest of them.
 */
reg_idx QbeCodegen::Add(reg_idx reg1, reg_idx reg2, PrimitiveType /*type*/)
{
    EmitBinary(reg1, "add", Register(reg1), Register(reg2));

    FreeRegister(reg2);

    return reg1;
}

reg_idx QbeCodegen::Sub(reg_idx reg1, reg_idx reg2, PrimitiveType /*type*/)
{
    EmitBinary(reg1, "sub", Register(reg1), Register(reg2));

    FreeRegister(reg2);

    return reg1;
}

reg_idx QbeCodegen::Mul(reg_idx reg1, reg_idx reg2, PrimitiveType /*type*/)
{
    EmitBinary(reg1, "mul", Register(reg1), Register(reg2));

    FreeRegister(reg2);

    return reg1;
}

reg_idx QbeCodegen::Div(reg_idx reg1, reg_idx reg2, PrimitiveType /*type*/)
{
    EmitBinary(reg1, "div", Register(reg1), Register(reg2));

    FreeRegister(reg2);

    return reg1;
}

/* What is left of the first value once the second has been taken from it as
 * many whole times as it holds. */
reg_idx QbeCodegen::Mod(reg_idx reg1, reg_idx reg2, PrimitiveType /*type*/)
{
    EmitBinary(reg1, "rem", Register(reg1), Register(reg2));

    FreeRegister(reg2);

    return reg1;
}

reg_idx QbeCodegen::And(reg_idx reg1, reg_idx reg2, PrimitiveType /*type*/)
{
    EmitBinary(reg1, "and", Register(reg1), Register(reg2));

    FreeRegister(reg2);

    return reg1;
}

reg_idx QbeCodegen::Or(reg_idx reg1, reg_idx reg2, PrimitiveType /*type*/)
{
    EmitBinary(reg1, "or", Register(reg1), Register(reg2));

    FreeRegister(reg2);

    return reg1;
}

reg_idx QbeCodegen::Xor(reg_idx reg1, reg_idx reg2, PrimitiveType /*type*/)
{
    EmitBinary(reg1, "xor", Register(reg1), Register(reg2));

    FreeRegister(reg2);

    return reg1;
}

reg_idx QbeCodegen::ShiftLeft(reg_idx reg1, reg_idx reg2,
    PrimitiveType /*type*/)
{
    EmitBinary(reg1, "shl", Register(reg1), Register(reg2));

    FreeRegister(reg2);

    return reg1;
}

/* A shift of a value to the right fills the top bits in with zeroes, which
 * is what the language asks for. */
reg_idx QbeCodegen::ShiftRight(reg_idx reg1, reg_idx reg2,
    PrimitiveType /*type*/)
{
    EmitBinary(reg1, "shr", Register(reg1), Register(reg2));

    FreeRegister(reg2);

    return reg1;
}

/*
 * Nothing has to be done to widen a value. A value is held as the whole of
 * it, with the bits above the type it has saying what it is, so a value of
 * a narrower type is already a value of the wider one.
 */
reg_idx QbeCodegen::Widen(reg_idx reg,
    PrimitiveType /*old_type*/,
    PrimitiveType /*new_type*/)
{
    return reg;
}

/*
 * A value is set aside under a name of its own, so that handing the
 * register out for another value leaves the value which was in it where it
 * was: nothing is written anywhere and the value is still there when it is
 * asked for again.
 */
void QbeCodegen::SpillRegister(reg_idx reg, int32 slot)
{
    std::string& aside = set_aside_[slot];

    aside = NewValue();
    ofs_ << "  " << aside << " =l copy " << Register(reg) << std::endl;
}

void QbeCodegen::ReloadRegister(reg_idx reg, int32 slot)
{
    ofs_ << "  " << Register(reg) << " =l copy " << set_aside_[slot];
    ofs_ << std::endl;
}

/*
 * Give a value the type it was cast to. A char holds the low eight bits of
 * what it is given, a short the low sixteen and an int the low thirty two,
 * each of them read back as a number of that type: a char has no sign, so
 * the bits which are not kept are zeroes, and a short and an int have one,
 * which is what the sign extension fills in. A wider type takes what it is
 * given as it stands.
 */
reg_idx QbeCodegen::Cast(reg_idx reg,
    PrimitiveType /*old_type*/,
    PrimitiveType new_type)
{
    switch (new_type)
    {
        case PrimitiveType::kChar:
            EmitBinary(reg, "and", Register(reg), "255");
            break;
        case PrimitiveType::kShort:
            EmitUnary(reg, "extsh");
            break;
        case PrimitiveType::kInt:
            EmitUnary(reg, "extsw");
            break;
        default:
            break;
    }

    return reg;
}

reg_idx QbeCodegen::Scale(reg_idx reg, int32 scale, PrimitiveType /*type*/)
{
    /* A size of one scales by one, which leaves the value alone. */
    if (scale == 1)
        return reg;

    EmitBinary(reg, "mul", Register(reg), std::to_string(scale));

    return reg;
}

void QbeCodegen::Move(reg_idx dest, reg_idx src, PrimitiveType /*type*/)
{
    ofs_ << "  " << Register(dest) << " =l copy " << Register(src) << std::endl;
}

reg_idx QbeCodegen::Negate(reg_idx reg, PrimitiveType /*type*/)
{
    EmitBinary(reg, "sub", "0", Register(reg));

    return reg;
}

reg_idx QbeCodegen::Invert(reg_idx reg, PrimitiveType /*type*/)
{
    EmitBinary(reg, "xor", Register(reg), "-1");

    return reg;
}

/* A value is false when it is zero, so the answer is the one the comparison
 * with zero leaves behind. */
reg_idx QbeCodegen::LogNot(reg_idx reg, PrimitiveType /*type*/)
{
    EmitBinary(reg, "ceql", Register(reg), "0");

    return reg;
}

reg_idx QbeCodegen::ToBool(reg_idx reg, PrimitiveType /*type*/)
{
    EmitBinary(reg, "cnel", Register(reg), "0");

    return reg;
}

/*
 * Add one step to the value a variable holds and store the new value back
 * where the variable is. Written after the variable the expression yields
 * the value it held before, written before it the value it holds now.
 */
reg_idx QbeCodegen::IncDec(const Symbol& symbol, int32 delta, bool post)
{
    reg_idx old_reg = LoadSymbol(symbol);
    reg_idx new_reg = AllocRegister();

    ofs_ << "  " << Register(new_reg) << " =l add " << Register(old_reg);
    ofs_ << ", " << delta << std::endl;

    StoreSymbol(symbol, new_reg);

    if (post)
    {
        FreeRegister(new_reg);
        return old_reg;
    }

    FreeRegister(old_reg);

    return new_reg;
}

reg_idx QbeCodegen::CompareAndSet(NodeTag op_type,
    reg_idx reg1,
    reg_idx reg2,
    PrimitiveType /*type*/)
{
    auto it = kComparisonInstr.find(op_type);

    if (it == kComparisonInstr.end())
    {
        std::cerr << "Error: token " << op_type;
        std::cerr << " is not a comparison!" << std::endl;
        std::exit(1);
    }

    EmitBinary(reg1, it->second.set, Register(reg1), Register(reg2));

    FreeRegister(reg2);

    return reg1;
}

/*
 * The jump is taken when the comparison does not hold, so the code which
 * follows only runs when the condition is true.
 */
void QbeCodegen::CompareAndJump(NodeTag op_type,
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

    const std::string test = NewValue();
    const label_idx next = NewLabel();

    ofs_ << "  " << test << " =l " << it->second.inverse << " ";
    ofs_ << Register(reg1) << ", " << Register(reg2) << std::endl;
    ofs_ << "  jnz " << test << ", " << Label(label) << ", ";
    ofs_ << Label(next) << std::endl;

    /*
     * The code which follows is what runs when the comparison holds, so it
     * stands under a label the jump goes around when it does not.
     */
    EmitLabel(next);

    /* The comparison is a jump and answers with nothing, so both values it
     * was given are done with. Only those two are freed: what the rest of
     * the expression holds is not this comparison's to throw away. */
    FreeRegister(reg1);
    FreeRegister(reg2);
}

void QbeCodegen::JumpIfEqual(reg_idx reg1,
    reg_idx reg2,
    label_idx label,
    PrimitiveType /*type*/)
{
    const std::string test = NewValue();
    const label_idx next = NewLabel();

    ofs_ << "  " << test << " =l ceql " << Register(reg1) << ", ";
    ofs_ << Register(reg2) << std::endl;
    ofs_ << "  jnz " << test << ", " << Label(label) << ", ";
    ofs_ << Label(next) << std::endl;

    /* Both values stay where they are: a switch asks each of its arms in
     * turn and keeps the value it is asking about. */
    EmitLabel(next);
}

void QbeCodegen::JumpIfZero(reg_idx reg, label_idx label)
{
    const std::string test = NewValue();
    const label_idx next = NewLabel();

    ofs_ << "  " << test << " =l ceql " << Register(reg) << ", 0" << std::endl;
    ofs_ << "  jnz " << test << ", " << Label(label) << ", ";
    ofs_ << Label(next) << std::endl;

    EmitLabel(next);
}

void QbeCodegen::JumpIfNonZero(reg_idx reg, label_idx label)
{
    const std::string test = NewValue();
    const label_idx next = NewLabel();

    ofs_ << "  " << test << " =l cnel " << Register(reg) << ", 0" << std::endl;
    ofs_ << "  jnz " << test << ", " << Label(label) << ", ";
    ofs_ << Label(next) << std::endl;

    EmitLabel(next);
}

/*
 * Each argument is given a name of its own as it is worked out, so that
 * handing its register out again while the rest of the arguments are worked
 * out leaves the value where it is.
 */
void QbeCodegen::PushArgument(reg_idx reg, PrimitiveType /*type*/)
{
    const std::string argument = NewValue();

    ofs_ << "  " << argument << " =l copy " << Register(reg) << std::endl;

    arguments_.push_back(argument);
}

/*
 * Call a function with the values which have been pushed. They were worked
 * out from the last one to the first and are given to the call the other
 * way round, which is the order the function takes them in.
 */
reg_idx QbeCodegen::Call(const Symbol& symbol, int32 arg_count)
{
    const std::size_t first = arguments_.size() - arg_count;

    reg_idx out_reg = AllocRegister();

    ofs_ << "  ";

    /* A function which answers with nothing is not called for a value. */
    if (symbol.type != PrimitiveType::kVoid)
        ofs_ << Register(out_reg) << " =l ";

    ofs_ << "call $" << symbol.name << "(";

    for (int32 k = 0; k < arg_count; k++)
    {
        if (k > 0)
            ofs_ << ", ";

        ofs_ << "l " << arguments_[first + arg_count - 1 - k];
    }

    ofs_ << ")" << std::endl;

    /* What this call was given is done with; what a call around it is
     * holding is still waiting. */
    arguments_.resize(first);

    return out_reg;
}

void QbeCodegen::Return(reg_idx reg)
{
    /*
     * A function answers with the value it was given. Whatever the type of
     * that value is, the whole of it is what the caller reads, which is
     * what every function of the language takes its arguments as well.
     */
    ofs_ << "  ret " << Register(reg) << std::endl;

    block_ended_ = true;
}

void QbeCodegen::PrintInt(reg_idx reg)
{
    ofs_ << "  call $" << kPrintIntName << "(l " << Register(reg) << ")";
    ofs_ << std::endl;
}

reg_idx QbeCodegen::AddressOf(const Symbol& symbol)
{
    reg_idx reg = AllocRegister();

    if (symbol.storage == StorageClass::kLocal)
    {
        ofs_ << "  " << Register(reg) << " =l copy %frame" << std::endl;
        AddOffset(reg, symbol.stack_offset);
        return reg;
    }

    /* A global variable is reached by the name of its storage. */
    ofs_ << "  " << Register(reg) << " =l copy $" << symbol.name << std::endl;

    return reg;
}

void QbeCodegen::AddOffset(reg_idx reg, int32 offset)
{
    if (offset == 0)
        return;

    EmitBinary(reg, "add", Register(reg), std::to_string(offset));
}

reg_idx QbeCodegen::LoadStrAddress(label_idx label)
{
    reg_idx reg = AllocRegister();

    ofs_ << "  " << Register(reg) << " =l copy $L" << label << std::endl;

    return reg;
}

/*
 * Read the value a pointer points at into the same register that holds the
 * pointer. How much is read is decided by what the pointer points at,
 * which is what makes reading through a pointer to a pointer read a whole
 * address.
 */
reg_idx QbeCodegen::Deref(reg_idx reg, PrimitiveType pointer_type)
{
    ofs_ << "  " << Register(reg) << " =l load";
    ofs_ << LoadLetter(PrimitiveSize(ValueAt(pointer_type)));
    ofs_ << " " << Register(reg) << std::endl;

    return reg;
}

reg_idx QbeCodegen::LoadDeref(reg_idx address_reg, PrimitiveType type)
{
    reg_idx value_reg = AllocRegister();

    ofs_ << "  " << Register(value_reg) << " =l load";
    ofs_ << LoadLetter(PrimitiveSize(type));
    ofs_ << " " << Register(address_reg) << std::endl;

    return value_reg;
}

void QbeCodegen::StoreDeref(reg_idx value_reg,
    reg_idx address_reg,
    PrimitiveType type)
{
    StoreAt(Register(address_reg), Register(value_reg), PrimitiveSize(type));
}

/*
 * The storage of a global variable, which QBE is told the whole of in one
 * statement: the elements which follow are written between the braces the
 * heading opens and the one EndGlobal() closes.
 */
void QbeCodegen::BeginGlobal(const Symbol& symbol, int32 alignment)
{
    if (!symbol.is_static)
        ofs_ << "export ";

    ofs_ << "data $" << symbol.name << " = align " << alignment << " { ";
}

/* The room a struct or a union takes, which starts out as zeroes however
 * much of it the program goes on to write. */
void QbeCodegen::EmitGlobalSpace(int32 size)
{
    ofs_ << "z " << size << ", ";
}

void QbeCodegen::EmitGlobalValue(int32 value, int32 size)
{
    ofs_ << SizeLetter(size) << " " << value << ", ";
}

/* An element which holds the address of a string holds the address the
 * label of that string stands for. */
void QbeCodegen::EmitGlobalLabelRef(label_idx label)
{
    ofs_ << "l $L" << label << ", ";
}

void QbeCodegen::EndGlobal()
{
    ofs_ << "}" << std::endl;
}

/*
 * A string holds one byte per character and then the NUL which ends it,
 * under a label of its own: unlike a global variable, a literal has no name
 * the rest of the program could refer to.
 *
 * A literal found while a function is being written cannot be written where
 * it stands, since QBE has the storage of the program apart from the
 * functions of it, so it waits for the end of that function.
 */
void QbeCodegen::GenStrStorage(label_idx label, const std::string& text)
{
    if (in_function_)
    {
        held_strings_.emplace_back(label, text);
        return;
    }

    WriteStringData(label, text);
}

void QbeCodegen::FreeAllRegister()
{
    AsmCodegen::FreeAllRegister();

    /*
     * Whatever was set aside belongs to the statement which set it aside,
     * so it is not brought back at the end of that statement.
     */
    for (idx_t slot = 0; slot < kSpillSlots; slot++)
        set_aside_[slot].clear();
}

/*
 * The widths of the language's types on the machine QBE writes for: a char
 * is one byte, a short two, an int four, a long eight and a pointer as wide
 * as one, which is what the machine this compiler runs on has as well.
 */
const TypeSizes& TargetSizes()
{
    static const TypeSizes kSizes{1, 2, 4, 8, 8};

    return kSizes;
}

std::string TargetName()
{
    return "qbe";
}

std::unique_ptr<AsmCodegen> MakeCodegen(const std::string& output_file)
{
    return std::make_unique<QbeCodegen>(output_file);
}

/* The target writes the intermediate language QBE reads, which QBE then
 * compiles into the assembly of the machine the program is built for. */
std::string CodeSuffix()
{
    return ".q";
}

std::string CodeTool()
{
    return "qbe";
}

/* What QBE writes is assembled and linked by the C compiler of the machine
 * the compiler runs on, as any program of that machine is. */
std::string CodeBuilder()
{
    return "";
}

std::optional<ToolCommand> BuildCommand(
    const std::vector<std::string>& /*code*/,
    const std::string& /*program*/)
{
    return std::nullopt;
}

/* What the generator wrote is what QBE reads, and nothing of the whole of
 * it is done to it: it is QBE which works the code over. */
bool OptimiseCode(const std::string& /*code*/)
{
    return true;
}

}   /* namespace nuocc */
