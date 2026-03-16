#pragma once

#include <fstream>
#include <memory>
#include <string>

#include "nodes/nuocc_ast_nodes.hpp"
#include "utils/nuocc_types.hpp"

namespace nuocc
{

constexpr int kRegSize = 4;

class AsmCodegen
{
public:
    AsmCodegen(const std::string& output_file);
    ~AsmCodegen();

public:
    void Preamble();
    void Postamble();

    void GenPrint(const AstNodePtr& root);
    reg_idx GenAstValue(const AstNodePtr& root);

    void GenGlobSymbol(const Symbol& symbol);
    reg_idx LoadGlobSymbol(const Symbol& symbol);
    reg_idx StoreGlobSymbol(const Symbol& symbol, reg_idx reg);

    void FreeRegister(reg_idx reg);

private:
    reg_idx AllocRegister();
    void FreeAllRegister();

    reg_idx Load(int32 value);
    reg_idx Add(reg_idx reg1, reg_idx reg2);
    reg_idx Sub(reg_idx reg1, reg_idx reg2);
    reg_idx Mul(reg_idx reg1, reg_idx reg2);
    reg_idx Div(reg_idx reg1, reg_idx reg2);
    void PrintInt(reg_idx reg);

    reg_idx GenAstOp(const AstNodePtr& root,
                     reg_idx left_reg,
                     reg_idx right_reg);

private:
    bool is_free_[kRegSize];
    std::string reg_list_[kRegSize];
    std::ofstream ofs_;
};

}   /* namespace nuocc */