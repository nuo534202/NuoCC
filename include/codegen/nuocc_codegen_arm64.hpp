#pragma once

#include <string>

#include "codegen/nuocc_codegen_assembly.hpp"
#include "nuocc_asm_codegen.hpp"

namespace nuocc
{

/*
 * The AArch64 target. Four of the callee saved registers hold values while
 * an expression is evaluated, so unlike x86-64 a call needs nothing saved
 * around it. The names are written the way Apple's assembler wants them,
 * see the note in the documentation.
 */
class Arm64Codegen : public AssemblyCodegen
{
public:
    explicit Arm64Codegen(const std::string& output_file);
    ~Arm64Codegen() override = default;

protected:
    void EmitPreamble() override;
    void EmitFunctionPreamble(
        const Symbol& symbol,
        int32 local_size,
        const std::vector<Symbol>& parameters) override;
    void EmitFunctionPostamble() override;
    void EmitLabel(label_idx label) override;
    void EmitJump(label_idx label) override;

    /*
     * Mach-O decorates the name of every global symbol with a leading
     * underscore, ELF leaves the name alone.
     */
    std::string GlobName(const std::string& name) const override;

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
    reg_idx ShiftLeft(reg_idx reg1, reg_idx reg2, PrimitiveType type) override;
    reg_idx ShiftRight(reg_idx reg1, reg_idx reg2, PrimitiveType type) override;
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

private:
    /* Load a register with the address of a global variable. */
    void EmitGlobAddress(const Symbol& symbol);
    /* Put a constant into a register, however many instructions it takes. */
    void EmitLoadImmediate(reg_idx reg, int64 value);

private:
    int32 frame_size_ = 0;

    /*
     * How far from x29 the places a value may be set aside begin, which is
     * past the locals of the function so that the two never overlap.
     */
    int32 spill_base_ = 0;

    std::string xreg_list_[kRegSize];
    std::string wreg_list_[kRegSize];
};

}   /* namespace nuocc */
