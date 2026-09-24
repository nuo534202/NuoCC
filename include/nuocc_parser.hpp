#pragma once

#include <string_view>
#include <unordered_map>
#include <vector>

#include "nodes/nuocc_ast_nodes.hpp"
#include "nodes/nuocc_nodes_tag.hpp"
#include "nodes/nuocc_nodes.hpp"
#include "utils/nuocc_symbol_table.hpp"
#include "utils/nuocc_types.hpp"

namespace nuocc
{

class Parser
{
private:
    /* Operator precedence: every binary operator has a precedence above 0 */
    static const std::unordered_map<NodeTag, uint8> kOpPrecedence;

public:
    /*
     * Parse the whole token list and return the abstract syntax tree of
     * the program. The program is a compound statement.
     */
    AstNodePtr Parse(const std::vector<NodePtr>& token_list);

private:
    AstNodePtr CompoundStatement(const std::vector<NodePtr>& token_list,
        idx_t& i);
    AstNodePtr Statement(const std::vector<NodePtr>& token_list,
        idx_t& i);

    AstNodePtr PrintStatement(const std::vector<NodePtr>& token_list,
        idx_t& i);
    AstNodePtr DeclareStatement(const std::vector<NodePtr>& token_list,
        idx_t& i);
    AstNodePtr AssignStatement(const std::vector<NodePtr>& token_list,
        idx_t& i);
    AstNodePtr IfStatement(const std::vector<NodePtr>& token_list,
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

    /*
     * The tag a token is matched by. A keyword carries its own tag inside
     * a T_KeyWord node, every other token is matched by its node tag.
     */
    static NodeTag TokenTag(const NodePtr& token);
    static bool IsComparisonOperator(NodeTag tag);

    void Match(const std::vector<NodePtr>& token_list,
               idx_t& i,
               NodeTag tag,
               std::string_view name);

    uint8 GetOpPrecedence(NodeTag tag);

private:
    SymbolTable symbol_table_;
};

}   /* namespace nuocc */
