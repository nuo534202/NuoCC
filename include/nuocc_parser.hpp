#pragma once

#include <optional>
#include <string_view>
#include <unordered_map>
#include <vector>

#include "nodes/nuocc_ast_nodes.hpp"
#include "nodes/nuocc_nodes_tag.hpp"
#include "nodes/nuocc_nodes.hpp"
#include "utils/nuocc_symbol_table.hpp"
#include "utils/nuocc_type_check.hpp"
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
     * Parse the whole token list and return the functions which make up
     * the program. A program is one or more function declarations.
     */
    std::vector<AstNodePtr> Parse(const std::vector<NodePtr>& token_list);

private:
    AstNodePtr FunctionDeclaration(const std::vector<NodePtr>& token_list,
        idx_t& i);

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
    AstNodePtr WhileStatement(const std::vector<NodePtr>& token_list,
        idx_t& i);
    AstNodePtr ForStatement(const std::vector<NodePtr>& token_list,
        idx_t& i);

    AstNodePtr Condition(const std::vector<NodePtr>& token_list,
        idx_t& i,
        std::string_view statement);
    void CheckComparison(const AstNodePtr& condition,
                         std::string_view statement);

    AstNodePtr BinaryExpression(
        const std::vector<NodePtr>& token_list,
        idx_t& i,
        uint8 ptp); /* previous token precedence */

    AstNodePtr ParsePrimary(const NodePtr& token);

    /* Parse the type which starts a declaration and step over it. */
    PrimitiveType ParseType(const std::vector<NodePtr>& token_list,
                            idx_t& i);

    /*
     * Widen the narrower of two operands so that both have the same type.
     */
    void WidenOperands(AstNodePtr& left, AstNodePtr& right);

    AstNodePtr MakeIntLitLeaf(const NodePtr& token, PrimitiveType type);
    AstNodePtr MakeIdentLeaf(const Symbol& symbol, bool is_lv_ident);
    AstNodePtr MakeOperatorNode(AstNodePtr& left,
                                AstNodePtr& right,
                                const NodePtr& node);
    AstNodePtr MakeWiden(AstNodePtr& expression, PrimitiveType type);

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
