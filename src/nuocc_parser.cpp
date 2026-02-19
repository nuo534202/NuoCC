#include "nuocc_parser.hpp"

#include <iostream>

#include "nodes/nuocc_scanner_nodes.hpp"

namespace nuocc
{

void Parser::Parse(const std::vector<std::unique_ptr<Node>>& token_list)
{
    size_t i = 0;
    ast_root_ = BinaryExpression(token_list, i, 0);
}

const std::unique_ptr<AstNode>& Parser::GetAstRoot()
{
    return ast_root_;
}

std::unique_ptr<AstNode> Parser::BinaryExpression(
    const std::vector<std::unique_ptr<Node>>& token_list,
    size_t& i,
    uint8 ptp) /* previous token precedence */
{
    std::unique_ptr<AstNode> left = ParsePrimary(token_list[i++]);
    
    if (token_list[i]->GetNodeTag() == T_EOF)
        return left;
    
    std::unique_ptr<AstNode> right = nullptr;
    NodeTag tag = token_list[i]->GetNodeTag();
    size_t root_index;

    while (GetOpPrecedence(tag) > ptp)
    {
        root_index = i;

        right = BinaryExpression(token_list, ++i, GetOpPrecedence(tag));
        left = MakeAstNode(left, right, token_list[root_index]);

        tag = token_list[i]->GetNodeTag();
        if (tag == T_EOF)
            break;
    }

    return left;
}

std::unique_ptr<AstNode>
Parser::ParsePrimary(const std::unique_ptr<Node>& token)
{
    switch (token->GetNodeTag())
    {
        case T_IntLit:
            return MakeAstLeaf(token);
        default:
            std::cerr << "syntax error!" << std::endl;
            exit(1);
    }

    return nullptr;
}

std::unique_ptr<AstNode>
Parser::MakeAstNode(std::unique_ptr<AstNode>& left,
                    std::unique_ptr<AstNode>& right,
                    const std::unique_ptr<Node>& node)
{
    switch (node->GetNodeTag())
    {
        case T_IntLit:
        {
            const Literal<int, T_IntLit>* intlit_node =
                static_cast<const Literal<int, T_IntLit>*>(node.get());
            return std::make_unique<AstIntLit>(left, right,
                                               intlit_node->GetValue());
        }
        case T_Plus:
        case T_Minus:
        case T_Star:
        case T_Slash:
            return std::make_unique<AstOperator>(left, right,
                                                 node->GetNodeTag());
        default:
            return nullptr;
    }

    return nullptr;
}

std::unique_ptr<AstNode>
Parser::MakeAstLeaf(const std::unique_ptr<Node>& node)
{
    std::unique_ptr<AstNode> left = nullptr;
    std::unique_ptr<AstNode> right = nullptr;

    return MakeAstNode(left, right, node);
}

std::unique_ptr<AstNode>
Parser::MakeAstUnary(std::unique_ptr<AstNode>& left,
                     const std::unique_ptr<Node>& node)
{
    std::unique_ptr<AstNode> right = nullptr;

    return MakeAstNode(left, right, node);
}

uint8 Parser::GetOpPrecedence(NodeTag tag)
{
    if (kOpPrecedence.find(tag) == kOpPrecedence.end())
    {
        std::cerr << "syntax error: operator not found!" << std::endl;
        exit(1);
    }

    return kOpPrecedence.at(tag);
}

const std::unordered_map<NodeTag, uint8> Parser::kOpPrecedence = {
    {T_IntLit, 1}, {T_EOF, 1},

    {T_Plus, 2}, {T_Minus, 2},

    {T_Star, 3}, {T_Slash, 3}
};

int32 InterpretAstTree(const std::unique_ptr<AstNode>& root)
{
    int32 left_value = 0, right_value = 0;

    if (root->GetLeft() != nullptr)
        left_value = InterpretAstTree(root->GetLeft());
    if (root->GetRight() != nullptr)
        right_value = InterpretAstTree(root->GetRight());
    
    switch (root->GetAstNodeTag())
    {
        case A_AstOperator:
            return InterpretAstOp(root, left_value, right_value);
        case A_AstIntLit:
        {
            const AstIntLit* int_lit =
                static_cast<const AstIntLit*>(root.get());
            return int_lit->GetValue();
        }
        default:
            break;
    }

    return 0;
}

int32 InterpretAstOp(const std::unique_ptr<AstNode>& root,
                     int32 left_value,
                     int32 right_value)
{
    const AstOperator* root_op =
        static_cast<AstOperator*>(root.get());
    
    switch (root_op->GetOpType())
    {
        case T_Plus:
            return left_value + right_value;
        case T_Minus:
            return left_value - right_value;
        case T_Star:
            return left_value * right_value;
        case T_Slash:
            return left_value / right_value;
        default:
            break;
    }

    return 0;
}

}   /* namespace nuocc */