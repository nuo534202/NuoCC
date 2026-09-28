#pragma once

#include <optional>
#include <string>

#include "nuocc_asm_codegen.hpp"

namespace nuocc
{

/*
 * The x86-64 target. Four of the eight byte registers are used to hold
 * values while an expression is evaluated; they are caller saved, so a
 * call has to have the ones still in use saved around it.
 */
class X86Codegen : public AsmCodegen
{
public:
    explicit X86Codegen(const std::string& output_file);
    ~X86Codegen() override = default;

protected:
    void EmitPreamble() override;
    void EmitFunctionPreamble(const Symbol& symbol,
                              int32 local_size,
                              const std::optional<Symbol>& parameter) override;
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
    reg_idx And(reg_idx reg1, reg_idx reg2) override;
    reg_idx Or(reg_idx reg1, reg_idx reg2) override;
    reg_idx Xor(reg_idx reg1, reg_idx reg2) override;
    reg_idx ShiftLeft(reg_idx reg1, reg_idx reg2) override;
    reg_idx ShiftRight(reg_idx reg1, reg_idx reg2) override;
    reg_idx Widen(reg_idx reg,
                  PrimitiveType old_type,
                  PrimitiveType new_type) override;
    reg_idx Scale(reg_idx reg, int32 scale) override;

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
    void JumpIfZero(reg_idx reg, label_idx label) override;

    reg_idx Call(const Symbol& symbol,
                 std::optional<reg_idx> arg_reg) override;
    void Return(reg_idx reg) override;
    void PrintInt(reg_idx reg) override;

    reg_idx AddressOf(const Symbol& symbol) override;
    reg_idx Deref(reg_idx reg, PrimitiveType pointer_type) override;
    void StoreDeref(reg_idx value_reg,
                    reg_idx address_reg,
                    PrimitiveType type) override;

private:
    /* Alter the value a variable holds in memory by one. */
    void EmitIncDecMemory(const Symbol& symbol, int32 delta);

    std::string reg_list_[kRegSize];
    /* The low byte of each register, required by the setX instructions. */
    std::string breg_list_[kRegSize];
    /* The low four bytes of each register, for the int type. */
    std::string dreg_list_[kRegSize];
};

}   /* namespace nuocc */
