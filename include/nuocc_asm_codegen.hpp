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
    void GenProgram(const std::vector<AstNodePtr>& functions);

protected:
    /* What every target has to provide. */

    virtual void EmitPreamble() = 0;
    virtual void EmitFunctionPreamble(const Symbol& symbol) = 0;
    virtual void EmitFunctionPostamble() = 0;
    virtual void EmitLabel(label_idx label) = 0;
    virtual void EmitJump(label_idx label) = 0;

    virtual void GenGlobSymbol(const Symbol& symbol) = 0;
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

    /* The parts of the walk which are the same everywhere. */

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
