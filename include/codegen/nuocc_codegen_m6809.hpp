#pragma once

#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include "nuocc_asm_codegen.hpp"

namespace nuocc
{

/*
 * The 6809 target. The machine works with one register, the D accumulator,
 * and every instruction which works out a value has one of its values in D
 * and the other one in memory. The four registers the walk of the tree
 * hands out are therefore four places in the frame of the function, and a
 * value is worked out in D and left in the place its register keeps it.
 * Everything the walk asks for which is not an instruction of the machine,
 * such as a multiplication or a division, is a call into the runtime every
 * program of this target is built with (lib/6809/runtime.s).
 *
 * The types of the language are the widths the machine gives them: a char
 * is one byte, a short, an int and a pointer two, and a long four. A short
 * is as wide as an int here, since the machine has no narrower number of
 * its own. A value is held in the low bytes of its register's place, big
 * endian, with the bytes above it saying what the value is: a char has
 * zeroes above it, since it has no sign, and a short has the byte which
 * says which half of the number it is.
 */
class M6809Codegen : public AsmCodegen
{
public:
    explicit M6809Codegen(const std::string& output_file);
    ~M6809Codegen() override = default;

protected:
    void EmitPreamble() override;
    void EmitEndOfProgram() override;
    void EmitFunctionPreamble(
        const Symbol& symbol,
        int32 local_size,
        const std::vector<Symbol>& parameters) override;
    void EmitFunctionPostamble() override;
    void EmitLabel(label_idx label) override;
    void EmitJump(label_idx label) override;

    reg_idx LoadInt(int32 value, PrimitiveType type) override;
    reg_idx LoadSymbol(const Symbol& symbol) override;
    reg_idx StoreSymbol(const Symbol& symbol, reg_idx reg) override;

    reg_idx Add(reg_idx reg1, reg_idx reg2, PrimitiveType type) override;
    reg_idx Sub(reg_idx reg1, reg_idx reg2, PrimitiveType type) override;
    reg_idx Mul(reg_idx reg1, reg_idx reg2, PrimitiveType type) override;
    reg_idx Div(reg_idx reg1, reg_idx reg2, PrimitiveType type) override;
    reg_idx Mod(reg_idx reg1, reg_idx reg2, PrimitiveType type) override;
    reg_idx And(reg_idx reg1, reg_idx reg2, PrimitiveType type) override;
    reg_idx Or(reg_idx reg1, reg_idx reg2, PrimitiveType type) override;
    reg_idx Xor(reg_idx reg1, reg_idx reg2, PrimitiveType type) override;
    reg_idx ShiftLeft(reg_idx reg1,
                      reg_idx reg2,
                      PrimitiveType type) override;
    reg_idx ShiftRight(reg_idx reg1,
                       reg_idx reg2,
                       PrimitiveType type) override;
    reg_idx Widen(reg_idx reg,
                  PrimitiveType old_type,
                  PrimitiveType new_type) override;

    void SpillRegister(reg_idx reg, int32 slot) override;
    void ReloadRegister(reg_idx reg, int32 slot) override;

    reg_idx Cast(reg_idx reg,
                 PrimitiveType old_type,
                 PrimitiveType new_type) override;
    reg_idx Scale(reg_idx reg, int32 scale, PrimitiveType type) override;
    void Move(reg_idx dest, reg_idx src, PrimitiveType type) override;

    reg_idx Negate(reg_idx reg, PrimitiveType type) override;
    reg_idx Invert(reg_idx reg, PrimitiveType type) override;
    reg_idx LogNot(reg_idx reg, PrimitiveType type) override;
    reg_idx ToBool(reg_idx reg, PrimitiveType type) override;
    reg_idx IncDec(const Symbol& symbol, int32 delta, bool post) override;

    reg_idx CompareAndSet(NodeTag op_type,
                          reg_idx reg1,
                          reg_idx reg2,
                          PrimitiveType type) override;
    void CompareAndJump(NodeTag op_type,
                        reg_idx reg1,
                        reg_idx reg2,
                        label_idx label,
                        PrimitiveType type) override;
    void JumpIfEqual(reg_idx reg1,
                     reg_idx reg2,
                     label_idx label,
                     PrimitiveType type) override;
    void JumpIfZero(reg_idx reg, label_idx label) override;
    void JumpIfNonZero(reg_idx reg, label_idx label) override;

    void PushArgument(reg_idx reg, PrimitiveType type) override;
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

private:
    /*
     * Where the four registers live in the frame of the function being
     * written, and how much room the whole frame takes: the locals of the
     * function, the four places of the registers, and the places a value
     * is set aside in when the walk asks for more registers than there
     * are.
     */
    int32 slot_base_ = 0;
    int32 frame_size_ = 0;
    /*
     * How much room the arguments of the call being made take on the
     * stack, from the last one to the first, which is the order they are
     * worked out in. A call within one of them adds its own and takes
     * them away again, so what the call around it is holding is left
     * where it is.
     */
    std::vector<int32> arguments_;
    /*
     * The storage of the string literals the program holds, which is
     * written after the last of the code: the code and the storage of a
     * program of this target stand in one run of bytes, so a literal put
     * where it is found would be run into.
     */
    std::vector<std::pair<label_idx, std::string>> strings_;

    /* The number of bytes one of the language's types takes. */
    static int32 SizeOf(PrimitiveType type);

    /*
     * The place in the frame one byte of a register's value is at, as the
     * machine is given an address: a positive offset from the stack
     * pointer, which points at the start of the frame.
     */
    std::string Slot(reg_idx reg, int32 byte) const;

    /* The place a value which is set aside is kept in. */
    std::string SpillSlot(int32 slot, int32 byte) const;

    /*
     * The names the storage of a string literal and a label are given. A
     * label carries the name of the file of code it is written in as well,
     * since every file of a program numbers its labels from one.
     */
    static std::string StringName(label_idx label);
    std::string Label(label_idx label) const;

    /*
     * What a name is written as in this program. A static name belongs to
     * the file it is written in, and the code of every file of a program
     * is put together into one, so it carries the name of the file it came
     * from.
     */
    std::string Name(const Symbol& symbol) const;

    /* Fill the high half of a value which is being widened, with zeroes
     * for a value which has no sign and with a copy of the sign bit for
     * one which has. */
    void EmitFillHighHalf(reg_idx reg, bool from_sign);

    /* The place a byte of a local variable is at, as the machine is given
     * an address. */
    static std::string Frame(int32 offset);

    /* Move a value of the given size between a place in the frame and a
     * register, and between a named place and a register. */
    void LoadFrame(reg_idx reg, int32 offset, int32 size);
    void StoreFrame(reg_idx reg, int32 offset, int32 size);
    void LoadNamed(reg_idx reg, const std::string& name, int32 size);
    void StoreNamed(reg_idx reg, const std::string& name, int32 size);

    /* Read the value the address in X stands for into a register, and
     * write the value a register holds through the address in X. */
    void LoadThrough(reg_idx reg, int32 size);
    void StoreThrough(reg_idx reg, int32 size);

    /* Move the bytes of a value from one register to another. */
    void CopyValue(reg_idx dest, reg_idx src, int32 size);

    /*
     * Compare the two values and jump to the label when the comparison
     * does not hold, which is what every condition becomes. The halves of
     * a four byte value are compared one after the other.
     */
    void EmitCompareJump(NodeTag op_type,
                         reg_idx reg1,
                         reg_idx reg2,
                         int32 size,
                         label_idx label);

    /* Jump to the label when the value is zero, which is the question a
     * condition which is not a comparison asks. */
    void EmitTestZero(reg_idx reg, int32 size, label_idx label);

    /* Work out one of the operations which is not an instruction of the
     * machine, by calling the runtime. */
    reg_idx EmitRuntimeBinary(reg_idx reg1,
                              reg_idx reg2,
                              PrimitiveType type,
                              std::string_view helper32,
                              std::string_view helper16);

    /* Shift the value of a register along by a number of bits which is not
     * known while the program is being compiled. */
    reg_idx EmitShift(reg_idx reg1, reg_idx reg2, int32 size, bool left);

    /* Add a value which is not known while the program is being compiled
     * to a value in a register. */
    void EmitAddConstant(reg_idx reg, int64 value, int32 size);
};

}   /* namespace nuocc */
