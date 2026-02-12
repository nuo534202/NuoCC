#include "utils/print.hpp"

#include <iostream>
#include <memory>
#include <string>
#include <vector>

#ifndef NDEBUG

/* Debug Mode */
namespace nuocc
{

#define PRINTTOKENLIST(scanner) PrintTokenList(scanner)

void PrintTokenList(const nuocc::Scanner& scanner)
{
    const std::vector<std::unique_ptr<Node>>& token_list
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
        else if (token->GetNodeTag() == T_Variable)
        {
            auto variable = static_cast<Variable*>(token.get());
            std::cout << " " << variable->GetName();
        }

        std::cout << std::endl;
    }
}

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
        case T_IntLit:
            out = "IntLit";
            break;
        case T_KeyWord:
            out = "KeyWord";
            break;
        case T_Int:
            out = "int";
            break;
        case T_Variable:
            out = "Variable";
            break;
        case T_Semicolon:
            out = ";";
            break;
        default:
            break;
    }

    return out;
}

}   /* namespace nuocc */

#endif