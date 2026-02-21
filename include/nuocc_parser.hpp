#pragma once

#include <memory>
#include <unordered_map>
#include <vector>

#include "nodes/nuocc_ast_nodes.hpp"
#include "nodes/nuocc_nodes_tag.hpp"
#include "nodes/nuocc_nodes.hpp"
#include "utils/nuocc_types.hpp"

namespace nuocc
{

class Parser
{
private:
    /* Operator Precedence: all larger than 0 */
    static const std::unordered_map<NodeTag, uint8> kOpPrecedence;

public:
    void Parse(const std::vector<std::unique_ptr<Node>>& token_list);
    const std::unique_ptr<AstNode>& GetAstRoot();

private:
    std::unique_ptr<AstNode> BinaryExpression(
        const std::vector<std::unique_ptr<Node>>& token_list,
        idx_t& i,
        uint8 ptp); /* previous token precedence */
    
    std::unique_ptr<AstNode> ParsePrimary(const std::unique_ptr<Node>& token);

    std::unique_ptr<AstNode> MakeAstNode(std::unique_ptr<AstNode>& left,
                                         std::unique_ptr<AstNode>& right,
                                         const std::unique_ptr<Node>& node);
    std::unique_ptr<AstNode> MakeAstLeaf(const std::unique_ptr<Node>& node);
    std::unique_ptr<AstNode> MakeAstUnary(std::unique_ptr<AstNode>& left,
                                          const std::unique_ptr<Node>& node);

    uint8 GetOpPrecedence(NodeTag tag);

private:
    std::unique_ptr<AstNode> ast_root_;
};

}   /* namespace nuocc */