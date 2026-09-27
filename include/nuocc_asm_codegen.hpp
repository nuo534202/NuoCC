#pragma once

#include <fstream>
#include <memory>
#include <string>
#include <vector>

#include "nodes/nuocc_ast_nodes.hpp"
#include "utils/nuocc_types.hpp"

namespace nuocc
{

constexpr int kRegSize = 4;

/*
 * The generic code generator. It walks the syntax tree of the program and
 * asks the target to emit the instructions, so everything which is not
 * specific to a machine lives here and only the instruction selection is
 * left to the subclasses.
 */
class AsmCodegen
{
public:
    explicit AsmCodegen(const std::string& output_file);
    virtual ~AsmCodegen();

public:
    /*
     * Generate the code of the whole program: the storage for the global
     * variables first, then the code of every function.
     */
    void GenProgram(const Program& program);

protected:
    /* What every target has to provide. */

    virtual void EmitPreamble() = 0;
    virtual void EmitFunctionPreamble(const Symbol& symbol) = 0;
    virtual void EmitFunctionPostamble() = 0;
    virtual void EmitLabel(label_idx label) = 0;
    virtual void EmitJump(label_idx label) = 0;

    /*
     * The name of a global symbol as the assembler of this target wants it
     * written. Mach-O decorates every global symbol with a leading
     * underscore, ELF leaves the name alone.
     */
    virtual std::string GlobName(const std::string& name) const;

    virtual reg_idx LoadInt(int32 value) = 0;
    virtual reg_idx LoadGlobSymbol(const Symbol& symbol) = 0;
    virtual reg_idx StoreGlobSymbol(const Symbol& symbol, reg_idx reg) = 0;

    virtual reg_idx Add(reg_idx reg1, reg_idx reg2) = 0;
    virtual reg_idx Sub(reg_idx reg1, reg_idx reg2) = 0;
    virtual reg_idx Mul(reg_idx reg1, reg_idx reg2) = 0;
    virtual reg_idx Div(reg_idx reg1, reg_idx reg2) = 0;
    virtual reg_idx Widen(reg_idx reg,
                          PrimitiveType old_type,
                          PrimitiveType new_type) = 0;
    virtual reg_idx Scale(reg_idx reg, int32 scale) = 0;

    virtual reg_idx CompareAndSet(NodeTag op_type,
                                  reg_idx reg1,
                                  reg_idx reg2) = 0;
    virtual void CompareAndJump(NodeTag op_type,
                                reg_idx reg1,
                                reg_idx reg2,
                                label_idx label) = 0;

    virtual reg_idx Call(const Symbol& symbol, reg_idx arg_reg) = 0;
    virtual void Return(reg_idx reg) = 0;
    virtual void PrintInt(reg_idx reg) = 0;

    virtual reg_idx AddressOf(const Symbol& symbol) = 0;
    virtual reg_idx Deref(reg_idx reg, PrimitiveType pointer_type) = 0;

    /* The parts of the walk which are the same everywhere. */

    /*
     * Reserve the storage of a global variable. The variables are laid out
     * one after another in the data section, in the order they are
     * declared, so that a program can reach one by adding an offset to the
     * address of another.
     */
    void GenGlobSymbol(const Symbol& symbol);

    void GenStatement(const AstNodePtr& root);
    void GenIf(const AstNodePtr& root);
    void GenWhile(const AstNodePtr& root);
    void GenCondition(const AstNodePtr& condition, label_idx false_label);
    reg_idx GenExpr(const AstNodePtr& root);
    reg_idx GenCall(const AstNodePtr& root);
    reg_idx GenOperator(NodeTag op_type, reg_idx left_reg, reg_idx right_reg);

    reg_idx AllocRegister();
    void FreeRegister(reg_idx reg);
    void FreeAllRegister();

    label_idx NewLabel();

    /* The registers which are free, which a target may want to know. */
    const bool* FreeRegisters() const;

protected:
    bool is_free_[kRegSize];
    label_idx next_label_;
    /* The function whose code is being generated. */
    label_idx function_end_label_;
    PrimitiveType function_return_type_;
    std::ofstream ofs_;
};

/*
 * Build the code generator for the machine this compiler was built for.
 * Each target implements it, the build picks one of them.
 */
std::unique_ptr<AsmCodegen> MakeCodegen(const std::string& output_file);

}   /* namespace nuocc */
