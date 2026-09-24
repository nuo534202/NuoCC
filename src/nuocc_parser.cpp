#include "nuocc_parser.hpp"

#include <stdlib.h>

#include <iostream>

#include "nodes/nuocc_scanner_nodes.hpp"

namespace nuocc
{

void Parser::Parse(const std::vector<NodePtr>& token_list)
{
    std::string output_file("asm_out.txt");
    nuocc::AsmCodegen asm_codegen(output_file);

    asm_codegen.Preamble();

    Statements(asm_codegen, token_list);

    asm_codegen.Postamble();
}

void Parser::Statements(AsmCodegen& asm_codegen,
    const std::vector<NodePtr>& token_list)
{
    idx_t i = 0;

    while (i < token_list.size())
    {
        switch(token_list[i]->GetNodeTag())
        {
            case T_KeyWord:
            {
                const KeyWord *key_word =
                    static_cast<const KeyWord *>(token_list[i].get());

                switch (key_word->GetWord())
                {
                    case T_Print:
                        PrintStatement(asm_codegen, token_list, i);
                        break;
                    case T_Int:
                        DeclareStatement(asm_codegen, token_list, i);
                        break;
                    default:
                        std::cerr << "syntax error: unknown keyword ";
                        std::cerr << key_word->GetWord() << "!" << std::endl;
                        exit(1);
                }
                break;
            }
            case T_Identifier:
                AssignStatement(asm_codegen, token_list, i);
                break;
            case T_EOF:
                return;
            default:
                std::cerr << "syntax error: incorrect token ";
                std::cerr << token_list[i]->GetNodeTag() << "!" << std::endl;
                exit(1);
        }
    }
}

void Parser::PrintStatement(AsmCodegen& asm_codegen,
    const std::vector<NodePtr>& token_list,
    idx_t& i)
{
    i++;

    AstNodePtr root;

    root = BinaryExpression(token_list, i, 0);

    if (token_list[i]->GetNodeTag() != T_Semicolon)
    {
        std::cerr << "syntax error: expect ;!" << std::endl;
        exit(1);
    }

    asm_codegen.GenPrint(root);

    i++;
}

void Parser::DeclareStatement(AsmCodegen& asm_codegen,
    const std::vector<NodePtr>& token_list,
    idx_t& i)
{
    i++;

    AstNodePtr root;

    if (token_list[i]->GetNodeTag() != T_Identifier)
    {
        std::cerr << "syntax error: expect an identifier!" << std::endl;
        exit(1);
    }

    const Identifier *ident =
        static_cast<const Identifier *>(token_list[i].get());
    Symbol symbol_name = ident->GetName();

    symbol_table_.AddSymbol(symbol_name);
    asm_codegen.GenGlobSymbol(symbol_name);

    i++;

    if (token_list[i]->GetNodeTag() != T_Semicolon)
    {
        std::cerr << "syntax error: expect ;!" << std::endl;
        exit(1);
    }

    i++;
}

void Parser::AssignStatement(AsmCodegen& asm_codegen,
    const std::vector<NodePtr>& token_list,
    idx_t& i)
{
    const Identifier *ident =
        static_cast<const Identifier *>(token_list[i].get());
    Symbol symbol_name = ident->GetName();
    idx_t sym_idx = symbol_table_.FindSymbol(symbol_name);

    if (!sym_idx)
    {
        std::cerr << "syntax error: undeclared variable " << symbol_name;
        std::cerr << "!" << std::endl;
        exit(1);
    }

    /* The identifier names the target of the assignment, it is an lvalue. */
    AstNodePtr right = MakeAstIdentLeaf(token_list[i], sym_idx, true);

    idx_t assign_idx = ++i;

    if (token_list[i]->GetNodeTag() != T_Assign)
    {
        std::cerr << "syntax error: expect =!" << std::endl;
        exit(1);
    }

    i++;

    AstNodePtr left = BinaryExpression(token_list, i, 0);
    AstNodePtr root = MakeAstNode(left, right, token_list[assign_idx]);

    reg_idx reg = asm_codegen.GenAstValue(root);
    asm_codegen.FreeRegister(reg);

    if (token_list[i]->GetNodeTag() != T_Semicolon)
    {
        std::cerr << "syntax error: expect ;!" << std::endl;
        exit(1);
    }

    i++;
}

AstNodePtr Parser::BinaryExpression(
    const std::vector<NodePtr>& token_list,
    idx_t& i,
    uint8 ptp) /* previous token precedence */
{
    AstNodePtr left = ParsePrimary(token_list[i++]);

    /*
     * Only a binary operator has a non-zero precedence, so the loop
     * stops as soon as a semicolon, an EOF or any other token shows up.
     */
    while (GetOpPrecedence(token_list[i]->GetNodeTag()) > ptp)
    {
        idx_t op_idx = i;
        uint8 op_prec = GetOpPrecedence(token_list[op_idx]->GetNodeTag());

        i++;

        AstNodePtr right = BinaryExpression(token_list, i, op_prec);
        left = MakeAstNode(left, right, token_list[op_idx]);
    }

    return left;
}

AstNodePtr
Parser::ParsePrimary(const NodePtr& token)
{
    switch (token->GetNodeTag())
    {
        case T_IntLit:
            return MakeAstLeaf(token);
        case T_Identifier:
        {
            const Identifier *ident =
                static_cast<const Identifier *>(token.get());
            idx_t sym_idx = symbol_table_.FindSymbol(ident->GetName());

            if (!sym_idx)
            {
                std::cerr << "syntax error: undeclared variable ";
                std::cerr << ident->GetName() << "!" << std::endl;
                std::exit(1);
            }

            return MakeAstIdentLeaf(token, sym_idx, false);
        }
        default:
            std::cerr << "syntax error!" << std::endl;
            std::exit(1);
    }

    return nullptr;
}

AstNodePtr Parser::MakeAstNode(AstNodePtr& left,
                               AstNodePtr& right,
                               const NodePtr& node)
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
        case T_Assign:
        case T_Plus:
        case T_Minus:
        case T_Star:
        case T_Slash:
            return std::make_unique<AstOperator>(left, right,
                                                 node->GetNodeTag());
        case T_Identifier:
            std::cerr << "code error: you should call MakeAstIdent" << std::endl;
            exit(1);
        default:
            return nullptr;
    }

    return nullptr;
}

AstNodePtr Parser::MakeAstLeaf(const NodePtr& node)
{
    AstNodePtr left = nullptr;
    AstNodePtr right = nullptr;

    return MakeAstNode(left, right, node);
}

AstNodePtr Parser::MakeAstUnary(AstNodePtr& left,
    const NodePtr& node)
{
    AstNodePtr right = nullptr;

    return MakeAstNode(left, right, node);
}

AstNodePtr Parser::MakeAstIdent(AstNodePtr& left,
    AstNodePtr& right,
    const NodePtr& node,
    idx_t ident_idx,
    bool is_lv_ident)
{
    const Identifier *ident =
        static_cast<const Identifier *>(node.get());
    return std::make_unique<AstIdentifier>(left,
        right, ident->GetName(), ident_idx, is_lv_ident);
}

AstNodePtr Parser::MakeAstIdentLeaf(const NodePtr& node,
    idx_t ident_idx,
    bool is_lv_ident)
{
    AstNodePtr left = nullptr, right = nullptr;
    return MakeAstIdent(left, right, node, ident_idx, is_lv_ident);
}

AstNodePtr Parser::MakeAstIdentUnary(AstNodePtr& left,
    const NodePtr& node,
    idx_t ident_idx,
    bool is_lv_ident)
{
    AstNodePtr right = nullptr;
    return MakeAstIdent(left, right, node, ident_idx, is_lv_ident);
}

uint8 Parser::GetOpPrecedence(NodeTag tag)
{
    auto it = kOpPrecedence.find(tag);

    /* Anything which is not a binary operator has zero precedence. */
    if (it == kOpPrecedence.end())
        return 0;

    return it->second;
}

const std::unordered_map<NodeTag, uint8> Parser::kOpPrecedence = {
    {T_Plus, 2}, {T_Minus, 2},

    {T_Star, 3}, {T_Slash, 3}
};

}   /* namespace nuocc */