#include "utils/nuocc_print.hpp"

#include <iostream>
#include <memory>
#include <string>
#include <vector>

namespace nuocc
{

std::string NodeTagToString(const NodeTag& tag)
{
    std::string out;    /* output */

    switch (tag)
    {
        case T_UnknownToken:
            out = "UnknownToken";
            break;
        case T_Plus:
            out = "+";
            break;
        case T_Minus:
            out = "-";
            break;
        case T_Star:
            out = "*";
            break;
        case T_Slash:
            out = "/";
            break;
        case T_Assign:
            out = "=";
            break;
        case T_EQ:
            out = "==";
            break;
        case T_NE:
            out = "!=";
            break;
        case T_LT:
            out = "<";
            break;
        case T_GT:
            out = ">";
            break;
        case T_LE:
            out = "<=";
            break;
        case T_GE:
            out = ">=";
            break;
        case T_IntLit:
            out = "IntLit";
            break;
        case T_KeyWord:
            out = "KeyWord";
            break;
        case T_Int:
            out = "int";
            break;
        case T_Print:
            out = "print";
            break;
        case T_If:
            out = "if";
            break;
        case T_Else:
            out = "else";
            break;
        case T_While:
            out = "while";
            break;
        case T_For:
            out = "for";
            break;
        case T_Identifier:
            out = "Identifier";
            break;
        case T_LBrace:
            out = "{";
            break;
        case T_RBrace:
            out = "}";
            break;
        case T_LParen:
            out = "(";
            break;
        case T_RParen:
            out = ")";
            break;
        case T_Semicolon:
            out = ";";
            break;
        case T_EOF:
            out = "EOF";
            break;
        default:
            break;
    }

    return out;
}

#ifndef NDEBUG

/* Debug Mode */
void PrintTokenList(const Scanner& scanner)
{
    const std::vector<NodePtr>& token_list
        = scanner.GetTokenList();

    for (auto& token : token_list)
    {
        std::cout << NodeTagToString(token->GetNodeTag());

        if (token->GetNodeTag() == T_IntLit)
        {
            auto lit = static_cast<Literal<int, T_IntLit>*>(token.get());
            std::cout << " " << lit->GetValue();
        }
        else if (token->GetNodeTag() == T_KeyWord)
        {
            auto key_word = static_cast<KeyWord*>(token.get());
            std::cout << " " << NodeTagToString(key_word->GetWord());
        }
        else if (token->GetNodeTag() == T_Identifier)
        {
            auto identifier = static_cast<Identifier*>(token.get());
            std::cout << " " << identifier->GetName();
        }

        std::cout << std::endl;
    }
}

#endif

}   /* namespace nuocc */
