#pragma once

#include <memory>
#include <unordered_map>
#include <vector>

#include "nodes/nuocc_ast_nodes.hpp"
#include "nodes/nuocc_nodes_tag.hpp"
#include "nodes/nuocc_nodes.hpp"
#include "utils/nuocc_symbol_table.hpp"
#include "utils/nuocc_types.hpp"
#include "nuocc_asm_codegen.hpp"

namespace nuocc
{

class Parser
{
private:
    /* Operator Precedence: all larger than 0 */
    static const std::unordered_map<NodeTag, uint8> kOpPrecedence;

public:
    void Parse(const std::vector<NodePtr>& token_list);

private:
    void Statements(AsmCodegen& asm_codegen,
        const std::vector<NodePtr>& token_list);
    void PrintStatement(AsmCodegen& asm_codegen,
        const std::vector<NodePtr>& token_list,
        idx_t& i);
    void DeclareStatement(AsmCodegen& asm_codegen,
        const std::vector<NodePtr>& token_list,
        idx_t& i);
    void AssignStatement(AsmCodegen& asm_codegen,
        const std::vector<NodePtr>& token_list,
        idx_t& i);

    AstNodePtr BinaryExpression(
        const std::vector<NodePtr>& token_list,
        idx_t& i,
        uint8 ptp); /* previous token precedence */
    
    AstNodePtr ParsePrimary(const NodePtr& token);

    /*
     *  You should use MakeAstIdent to make an AstIdentifier node
     */
    AstNodePtr MakeAstNode(AstNodePtr& left,
                           AstNodePtr& right,
                           const NodePtr& node);
    AstNodePtr MakeAstLeaf(const NodePtr& node);
    AstNodePtr MakeAstUnary(AstNodePtr& left,
                            const NodePtr& node);

    AstNodePtr MakeAstIdent(AstNodePtr& left,
                            AstNodePtr& right,
                            const NodePtr& node,
                            idx_t ident_idx,
                            bool is_lv_ident);
    AstNodePtr MakeAstIdentLeaf(const NodePtr& node,
                                idx_t ident_idx,
                                bool is_lv_ident);
    AstNodePtr MakeAstIdentUnary(AstNodePtr& left,
                                 const NodePtr& node,
                                 idx_t ident_idx,
                                 bool is_lv_ident);

    uint8 GetOpPrecedence(NodeTag tag);

private:
    SymbolTable symbol_table_;
};

}   /* namespace nuocc */