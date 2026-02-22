#include "nuocc_scanner.hpp"

#include <fstream>
#include <iostream>

namespace nuocc
{

void Scanner::Scan(const std::string& file)
{
    std::ifstream ifs;

    ifs.open(file.c_str(), std::ios::in);
    if (!ifs.is_open())
    {
        std::cerr << "Error: Fail to open file " << file << std::endl;
        return;
    }

    std::string buf;

    while (ifs >> buf)
    {
        idx_t i = 0;
        StringToToken(buf, i);
    }

    token_list_.push_back(std::make_unique<Node>(T_EOF));
    ifs.close();
}

const std::vector<std::unique_ptr<Node>>& Scanner::GetTokenList() const
{
    return token_list_;
}

void Scanner::StringToToken(const std::string& buf, idx_t& i)
{
    SkipEmpty(buf, i);

    size_t size = buf.size();
    std::string token;

    for (; i < size; i++)
    {
        /* character is not in the alphabet */
        if (alphabet.find(buf[i]) == alphabet.end())
        {
            std::cerr << "Error: unrecognized character!" << buf[i] << std::endl;
            return;
        }

        if (IsNewToken(token, buf[i]))
        {
            CommitToken(token);
            BeginToken(token, buf[i]);
            continue;
        }

        AppendToken(token, buf[i]);
    }

    CommitToken(token);
}

void Scanner::SkipEmpty(const std::string& buf, idx_t& i)
{
    size_t size = buf.size();

    while (i < size)
    {
        if (buf[i] == ' ' ||
            buf[i] == '\n' ||
            buf[i] == '\t')
        {
            i++;
        }
        else
        {
            break;
        }
    }
}

bool Scanner::IsNewToken(const std::string& token, char ch)
{
    return single_op.find(ch) != single_op.end() ||
        (token.size() == 1 &&
         single_op.find(token.front()) != single_op.end());
}

void Scanner::BeginToken(std::string& token, char ch)
{
    token = ch;
}

void Scanner::AppendToken(std::string& token, char ch)
{
    token.push_back(ch);
}

void Scanner::CommitToken(const std::string& token)
{
    if (token.empty())
        return;

    NodeTag nodetag = GetTokenNodeTag(token);
    std::unique_ptr<Node> token_node;

    switch (nodetag)
    {
        case T_KeyWord:
            token_node = std::make_unique<KeyWord>(key_words.at(token));
            break;

        case T_IntLit:
            token_node = std::make_unique<Literal<int, T_IntLit>>(std::stoi(token));
            break;

        case T_Variable:
            token_node = std::make_unique<Variable>(token);
            break;

        case T_Plus:
        case T_Minus:
        case T_Star:
        case T_Slash:
        case T_Assign:
        case T_Semicolon:
            token_node = std::make_unique<Node>(nodetag);
            break;

        case T_UnknownToken:
            std::cerr << "lexical error: unknown token!" << std::endl;
            std::exit(1);
        default:
            token_node = std::make_unique<Node>(T_UnknownToken);
            break;
    }

    if (token_node)
        token_list_.push_back(std::move(token_node));
}

NodeTag Scanner::GetTokenNodeTag(const std::string& token)
{
    if (token.size() == 1 && token.front() == ';')
        return T_Semicolon;

    if (token.size() == 1 && single_op.find(token.front()) != single_op.end())
        return single_op.at(token.front());

    if (key_words.find(token) != key_words.end())
        return T_KeyWord;

    if (IsIntLit(token))
        return T_IntLit;

    if (IsVariable(token))
        return T_Variable;
    
    return T_UnknownToken;
}

bool Scanner::IsIntLit(const std::string& token)
{
    for (char c : token)
    {
        if (!isdigit(c))
            return false;
    }

    return true;
}

bool Scanner::IsVariable(const std::string& token)
{
    for (idx_t i = 0; i < token.size(); i++)
    {
        if (i == 0 && isdigit(token[i]))
            return false;
        
        if (token[i] != '_' && !isalpha(token[i]) && !isdigit(token[i]))
            return false;
    }

    return true;
}

const std::unordered_map<std::string, NodeTag> Scanner::key_words = {
    {"int", T_Int}
};

const std::unordered_map<char, NodeTag> Scanner::single_op = {
    {'+', T_Plus}, {'-', T_Minus}, {'*', T_Star}, {'/', T_Slash},
    {'=', T_Assign}, {';', T_Semicolon}
};

const std::unordered_set<char> Scanner::alphabet = {
    '+', '-', '*', '/', '=', ';', '_', '.',

    '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',

    'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j',
    'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's', 't',
    'u', 'v', 'w', 'x', 'y', 'z',

    'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J',
    'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T',
    'U', 'V', 'W', 'X', 'Y', 'Z'
};

}   /* namespace nuocc */