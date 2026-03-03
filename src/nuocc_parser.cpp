#include "nuocc_parser.hpp"

#include <stdlib.h>

#include <iostream>

#include "nodes/nuocc_scanner_nodes.hpp"

namespace nuocc
{

void Parser::Parse(const std::vector<std::unique_ptr<Node>>& token_list)
{
    std::string output_file("asm_out.txt");
    nuocc::AsmCodegen asm_codegen(output_file);

    asm_codegen.Preamble();

    Statement(asm_codegen, token_list);

    asm_codegen.Postamble();
}

void Parser::Statement(
    AsmCodegen& asm_codegen,
    const std::vector<std::unique_ptr<Node>>& token_list)
{
    std::unique_ptr<AstNode> root;
    idx_t i = 0;

    while (true)
    {
        if (token_list[i]->GetNodeTag() != T_KeyWord)
        {
            std::cerr << "syntax error: expected key word print!" << std::endl;
            exit(1);
        }
        else
        {
            const KeyWord *key_word =
                static_cast<const KeyWord *>(token_list[i].get());
            if (key_word->GetWord() != T_Print)
            {
                std::cerr << "syntax error: expected key word print!" << std::endl;
                exit(1);
            }
        }

        i++;

        root = BinaryExpression(token_list, i, 0);

        if (token_list[i]->GetNodeTag() != T_Semicolon)
        {
            std::cerr << "syntax error: expected ;" << std::endl;
            exit(1);
        }

        i++;

        asm_codegen.GenPrint(root);

        if (token_list[i]->GetNodeTag() == T_EOF)
            break;
    }
}

std::unique_ptr<AstNode> Parser::BinaryExpression(
    const std::vector<std::unique_ptr<Node>>& token_list,
    idx_t& i,
    uint8 ptp) /* previous token precedence */
{
    std::unique_ptr<AstNode> left = ParsePrimary(token_list[i++]);
    
    NodeTag tag = token_list[i]->GetNodeTag();
    if (tag == T_Semicolon)
        return left;
    
    std::unique_ptr<AstNode> right = nullptr;
    idx_t root_index;

    while (GetOpPrecedence(tag) > ptp)
    {
        root_index = i;

        right = BinaryExpression(token_list, ++i, GetOpPrecedence(tag));
        left = MakeAstNode(left, right, token_list[root_index]);

        tag = token_list[i]->GetNodeTag();
        if (tag == T_Semicolon)
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
            std::exit(1);
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
        std::exit(1);
    }

    return kOpPrecedence.at(tag);
}

const std::unordered_map<NodeTag, uint8> Parser::kOpPrecedence = {
    {T_IntLit, 1}, {T_EOF, 1},

    {T_Plus, 2}, {T_Minus, 2},

    {T_Star, 3}, {T_Slash, 3}
};

}   /* namespace nuocc */