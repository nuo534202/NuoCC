#pragma once

#include <fstream>
#include <string>
#include <string_view>
#include <vector>

#include "nodes/nuocc_ast_nodes.hpp"
#include "utils/nuocc_types.hpp"

namespace nuocc
{

constexpr int kRegSize = 4;

/*
 * Code generator for x86-64. It walks the abstract syntax tree of the
 * whole program and writes the assembly code for it.
 */
class AsmCodegen
{
public:
    explicit AsmCodegen(const std::string& output_file);
    ~AsmCodegen();

public:
    void GenProgram(const std::vector<AstNodePtr>& functions);

private:
    void GenPreamble();
    void GenFunction(const AstNodePtr& root);
    void GenFunctionPreamble(const Symbol& name);
    void GenFunctionPostamble();

    void GenStatement(const AstNodePtr& root);
    void GenIf(const AstNodePtr& root);
    void GenWhile(const AstNodePtr& root);
    void GenCondition(const AstNodePtr& condition, label_idx false_label);
    reg_idx GenExpr(const AstNodePtr& root);
    reg_idx GenCall(const AstNodePtr& root);
    void GenReturn(reg_idx reg);

    void GenGlobSymbol(const Symbol& symbol);
    reg_idx LoadGlobSymbol(const Symbol& symbol);
    reg_idx StoreGlobSymbol(const Symbol& symbol, reg_idx reg);

    void FreeRegister(reg_idx reg);

private:
    reg_idx AllocRegister();
    void FreeAllRegister();

    label_idx NewLabel();
    void EmitLabel(label_idx label);
    void EmitJump(label_idx label);

    reg_idx Load(int32 value);
    reg_idx Add(reg_idx reg1, reg_idx reg2);
    reg_idx Sub(reg_idx reg1, reg_idx reg2);
    reg_idx Mul(reg_idx reg1, reg_idx reg2);
    reg_idx Div(reg_idx reg1, reg_idx reg2);
    reg_idx Widen(reg_idx reg,
                  PrimitiveType old_type,
                  PrimitiveType new_type);

    reg_idx CompareAndSet(NodeTag op_type, reg_idx reg1, reg_idx reg2);
    void CompareAndJump(NodeTag op_type,
                        reg_idx reg1,
                        reg_idx reg2,
                        label_idx label);

    void PrintInt(reg_idx reg);

    reg_idx GenOperator(NodeTag op_type,
                        reg_idx left_reg,
                        reg_idx right_reg);

private:
    bool is_free_[kRegSize];
    std::string reg_list_[kRegSize];
    /* The low byte of each register, required by the setX instructions. */
    std::string breg_list_[kRegSize];
    /* The low four bytes of each register, for the int type. */
    std::string dreg_list_[kRegSize];
    label_idx next_label_;
    /* The function whose code is being generated. */
    label_idx function_end_label_;
    PrimitiveType function_return_type_;
    std::ofstream ofs_;
};

}   /* namespace nuocc */
