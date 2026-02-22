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
    void GenerateAsmCode(const std::unique_ptr<AstNode>& root);

private:
    void Preamble();
    void Postamble();

    reg_idx AllocRegister();
    void FreeRegister(reg_idx reg);
    void FreeAllRegister();

    reg_idx Load(int32 value);
    reg_idx Add(reg_idx reg1, reg_idx reg2);
    reg_idx Sub(reg_idx reg1, reg_idx reg2);
    reg_idx Mul(reg_idx reg1, reg_idx reg2);
    reg_idx Div(reg_idx reg1, reg_idx reg2);
    void PrintInt(reg_idx reg);

    reg_idx GenAst(const std::unique_ptr<AstNode>& root);
    reg_idx GenAstOp(const std::unique_ptr<AstNode>& root,
                     reg_idx left_reg,
                     reg_idx right_reg);

private:
    bool is_free_[kRegSize];
    std::string reg_list_[kRegSize];
    std::ofstream ofs_;
};

}   /* namespace nuocc */