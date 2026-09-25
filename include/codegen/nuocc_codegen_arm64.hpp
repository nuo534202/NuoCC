#pragma once

#include <string>

#include "nuocc_asm_codegen.hpp"

namespace nuocc
{

/*
 * The AArch64 target. Four of the callee saved registers hold values while
 * an expression is evaluated, so unlike x86-64 a call needs nothing saved
 * around it. The names are written the way Apple's assembler wants them,
 * see the note in the documentation.
 */
class Arm64Codegen : public AsmCodegen
{
public:
    explicit Arm64Codegen(const std::string& output_file);
    ~Arm64Codegen() override = default;

protected:
    void EmitPreamble() override;
    void EmitFunctionPreamble(const Symbol& symbol) override;
    void EmitFunctionPostamble() override;
    void EmitLabel(label_idx label) override;
    void EmitJump(label_idx label) override;

    void GenGlobSymbol(const Symbol& symbol) override;
    reg_idx LoadInt(int32 value) override;
    reg_idx LoadGlobSymbol(const Symbol& symbol) override;
    reg_idx StoreGlobSymbol(const Symbol& symbol, reg_idx reg) override;

    reg_idx Add(reg_idx reg1, reg_idx reg2) override;
    reg_idx Sub(reg_idx reg1, reg_idx reg2) override;
    reg_idx Mul(reg_idx reg1, reg_idx reg2) override;
    reg_idx Div(reg_idx reg1, reg_idx reg2) override;
    reg_idx Widen(reg_idx reg,
                  PrimitiveType old_type,
                  PrimitiveType new_type) override;

    reg_idx CompareAndSet(NodeTag op_type,
                          reg_idx reg1,
                          reg_idx reg2) override;
    void CompareAndJump(NodeTag op_type,
                        reg_idx reg1,
                        reg_idx reg2,
                        label_idx label) override;

    reg_idx Call(const Symbol& symbol, reg_idx arg_reg) override;
    void Return(reg_idx reg) override;
    void PrintInt(reg_idx reg) override;

private:
    /* Load a register with the address of a global variable. */
    void EmitGlobAddress(const Symbol& symbol);
    /* Put a constant into a register, however many instructions it takes. */
    void EmitLoadImmediate(reg_idx reg, int64 value);

private:
    std::string xreg_list_[kRegSize];
    std::string wreg_list_[kRegSize];
};

}   /* namespace nuocc */
