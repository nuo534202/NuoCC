#pragma once

#include <string>
#include <string_view>
#include <vector>

#include "nuocc_asm_codegen.hpp"

namespace nuocc
{

/*
 * The QBE target. Rather than writing the assembly of one machine, it
 * writes the intermediate language which QBE reads, and QBE compiles that
 * down to the assembly of whichever machine the program is built for; the
 * driver runs QBE over the file this target writes and hands what QBE
 * wrote to the assembler and linker.
 *
 * QBE has no registers of its own, so the four registers this compiler
 * hands out are four names QBE is told about: %r0 to %r3. A register is
 * given a value as often as the walk asks for one, which is what QBE's own
 * SSA pass is there to sort out, and it is what lets a value written into
 * a register on both sides of a branch be read after the two of them meet.
 * The values which are not registers, such as the answer to a comparison
 * or the address of a place in the frame, are given names of their own
 * which nothing else is written with.
 *
 * QBE has one kind of value of the whole width of a register, which is
 * what this target holds every value as: a char is read without a sign
 * and an int with one, so the bits above a narrow value already say what
 * it is, and an operation on two values means the same thing whatever
 * their declared type is.
 *
 * The storage of a string literal is the one thing which cannot be
 * written where it is found: QBE has the storage of the program and the
 * functions of it apart, and a literal is found while the function which
 * uses it is being written. What it takes is remembered and written after
 * the function, where it stands with the rest of the storage.
 */
class QbeCodegen : public AsmCodegen
{
public:
    explicit QbeCodegen(const std::string& output_file);
    ~QbeCodegen() override = default;

protected:
    void EmitPreamble() override;
    void EmitFunctionPreamble(
        const Symbol& symbol,
        int32 local_size,
        const std::vector<Symbol>& parameters) override;
    void EmitFunctionPostamble() override;
    void EmitLabel(label_idx label) override;
    void EmitJump(label_idx label) override;

    reg_idx LoadInt(int32 value) override;
    reg_idx LoadSymbol(const Symbol& symbol) override;
    reg_idx StoreSymbol(const Symbol& symbol, reg_idx reg) override;

    reg_idx Add(reg_idx reg1, reg_idx reg2) override;
    reg_idx Sub(reg_idx reg1, reg_idx reg2) override;
    reg_idx Mul(reg_idx reg1, reg_idx reg2) override;
    reg_idx Div(reg_idx reg1, reg_idx reg2) override;
    reg_idx Mod(reg_idx reg1, reg_idx reg2) override;
    reg_idx And(reg_idx reg1, reg_idx reg2) override;
    reg_idx Or(reg_idx reg1, reg_idx reg2) override;
    reg_idx Xor(reg_idx reg1, reg_idx reg2) override;
    reg_idx ShiftLeft(reg_idx reg1, reg_idx reg2) override;
    reg_idx ShiftRight(reg_idx reg1, reg_idx reg2) override;
    reg_idx Widen(reg_idx reg,
                  PrimitiveType old_type,
                  PrimitiveType new_type) override;

    void SpillRegister(reg_idx reg, int32 slot) override;
    void ReloadRegister(reg_idx reg, int32 slot) override;

    reg_idx Cast(reg_idx reg,
                 PrimitiveType old_type,
                 PrimitiveType new_type) override;
    reg_idx Scale(reg_idx reg, int32 scale) override;
    void Move(reg_idx dest, reg_idx src) override;

    reg_idx Negate(reg_idx reg) override;
    reg_idx Invert(reg_idx reg) override;
    reg_idx LogNot(reg_idx reg) override;
    reg_idx ToBool(reg_idx reg) override;
    reg_idx IncDec(const Symbol& symbol, int32 delta, bool post) override;

    reg_idx CompareAndSet(NodeTag op_type,
                          reg_idx reg1,
                          reg_idx reg2) override;
    void CompareAndJump(NodeTag op_type,
                        reg_idx reg1,
                        reg_idx reg2,
                        label_idx label) override;
    void JumpIfEqual(reg_idx reg1,
                     reg_idx reg2,
                     label_idx label) override;
    void JumpIfZero(reg_idx reg, label_idx label) override;
    void JumpIfNonZero(reg_idx reg, label_idx label) override;

    void PushArgument(reg_idx reg) override;
    reg_idx Call(const Symbol& symbol, int32 arg_count) override;
    void Return(reg_idx reg) override;
    void PrintInt(reg_idx reg) override;

    reg_idx AddressOf(const Symbol& symbol) override;
    void AddOffset(reg_idx reg, int32 offset) override;
    reg_idx LoadStrAddress(label_idx label) override;
    reg_idx Deref(reg_idx reg, PrimitiveType pointer_type) override;
    reg_idx LoadDeref(reg_idx address_reg, PrimitiveType type) override;
    void StoreDeref(reg_idx value_reg,
                    reg_idx address_reg,
                    PrimitiveType type) override;

    void BeginGlobal(const Symbol& symbol, int32 alignment) override;
    void EmitGlobalSpace(int32 size) override;
    void EmitGlobalValue(int32 value, int32 size) override;
    void EmitGlobalLabelRef(label_idx label) override;
    void EndGlobal() override;
    void GenStrStorage(label_idx label, const std::string& text) override;

    void FreeAllRegister() override;

private:
    /*
     * The value each of the places the walk keeps for the registers holds:
     * the value itself is kept under the name QBE gave it, so setting one
     * aside is remembering that name rather than writing the value
     * anywhere.
     */
    std::string set_aside_[kSpillSlots];
    /*
     * The values the call being made is to be given, from the last
     * argument to the first, which is the order they are worked out in. A
     * call within one of them adds its own values and takes them away
     * again, so what the call around it is holding is left where it is.
     */
    std::vector<std::string> arguments_;
    /* How many values the function being written has been given names for. */
    int32 temporary_count_ = 0;
    /* Whether a function is being written, which is where the storage of a
     * string literal may not be written. */
    bool in_function_ = false;
    /* The storage of the string literals found while the function being
     * written is written, waiting for it to be over. */
    std::vector<std::pair<label_idx, std::string>> held_strings_;

    /* A name QBE has not seen before, for the value just worked out. */
    std::string NewValue();

    /* The name of one of the four registers. */
    static std::string Register(reg_idx reg);

    /*
     * Work out one operation from two values and leave the result in a
     * register, over whatever it held before. The operation is written
     * with the width this target holds every value as, so the two values
     * are the two long words the instruction takes.
     */
    void EmitBinary(reg_idx reg,
                    std::string_view op,
                    const std::string& left,
                    const std::string& right);

    /* The same for an operation which takes one value. */
    void EmitUnary(reg_idx reg, std::string_view op);

    /* The address of a place in the frame of the function being written. */
    std::string FrameAddress(int32 offset);

    /* Write a value to an address, taking as much room as the size says. */
    void StoreAt(const std::string& address,
                 const std::string& value,
                 int32 size);

    /* Write the storage of one string literal, where the storage of the
     * program is written. */
    void WriteStringData(label_idx label, const std::string& text);

    /* Write the storage of the string literals the function being written
     * held, which is done once it is over. */
    void WriteHeldStrings();

    /* The letters which say of what width the operations of a size are
     * written, and the name of a label of this target. */
    static char SizeLetter(int32 size);
    static const char* LoadLetter(int32 size);
    static std::string Label(label_idx label);
};

}   /* namespace nuocc */
