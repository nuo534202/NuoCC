#include "nuocc_scanner.hpp"

#include <cctype>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <limits>
#include <stdexcept>

namespace nuocc
{

void Scanner::Scan(const std::string& file)
{
    std::ifstream ifs;

    ifs.open(file.c_str(), std::ios::in);
    if (!ifs.is_open())
    {
        std::cerr << "Error: Fail to open file " << file << std::endl;
        std::exit(1);
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

const std::vector<NodePtr>& Scanner::GetTokenList() const
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
        if (kAlphabet.find(buf[i]) == kAlphabet.end())
        {
            std::cerr << "Error: unrecognized character " << buf[i];
            std::cerr << "!" << std::endl;
            std::exit(1);
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
    /*
     * The next character may still be part of the token when the two of
     * them together start a two character operator such as == or <=.
     */
    if (kDoubleOp.find(token + ch) != kDoubleOp.end())
        return false;

    /* An operator is at most two characters long, so it ends here. */
    if (kDoubleOp.find(token) != kDoubleOp.end())
        return true;

    /* ch is a single op, or token is a single op*/
    if (kSingleOp.find(ch) != kSingleOp.end() ||
        (token.size() == 1 &&
         kSingleOp.find(token.front()) != kSingleOp.end()))
        return true;

    return false;
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
    NodePtr token_node;

    switch (nodetag)
    {
        case T_KeyWord:
            token_node = std::make_unique<KeyWord>(kKeyWords.at(token));
            break;

        case T_IntLit:
            token_node = std::make_unique<Literal<int, T_IntLit>>(
                ToIntLit(token));
            break;

        case T_Identifier:
            token_node = std::make_unique<Identifier>(token);
            break;

        case T_Plus:
        case T_Minus:
        case T_Star:
        case T_Slash:
        case T_Assign:
        case T_Semicolon:
        case T_EQ:
        case T_NE:
        case T_LT:
        case T_GT:
        case T_LE:
        case T_GE:
        case T_LBrace:
        case T_RBrace:
        case T_LParen:
        case T_RParen:
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
    if (token.size() == 1 && kSingleOp.find(token.front()) != kSingleOp.end())
        return kSingleOp.at(token.front());

    if (token.size() == 2 && kDoubleOp.find(token) != kDoubleOp.end())
        return kDoubleOp.at(token);

    if (kKeyWords.find(token) != kKeyWords.end())
        return T_KeyWord;

    if (IsIntLit(token))
        return T_IntLit;

    if (IsIdent(token))
        return T_Identifier;

    return T_UnknownToken;
}

bool Scanner::IsIntLit(const std::string& token)
{
    if (token.empty())
        return false;

    for (char c : token)
    {
        if (!isdigit(static_cast<unsigned char>(c)))
            return false;
    }

    return true;
}

/*
 * Convert a token made of digits into its value. The language has no
 * integer type wider than int yet, so a literal which does not fit is an
 * error rather than a silent overflow.
 */
int32 Scanner::ToIntLit(const std::string& token)
{
    long long value = 0;
    bool in_range = true;

    try
    {
        value = std::stoll(token);
    }
    catch (const std::out_of_range&)
    {
        in_range = false;
    }

    if (!in_range || value > std::numeric_limits<int32>::max())
    {
        std::cerr << "lexical error: integer literal " << token;
        std::cerr << " is too large!" << std::endl;
        std::exit(1);
    }

    return static_cast<int32>(value);
}

bool Scanner::IsIdent(const std::string& token)
{
    if (token.empty())
        return false;

    if (isdigit(static_cast<unsigned char>(token.front())))
        return false;

    for (char c : token)
    {
        bool is_valid = c == '_' ||
                        isalpha(static_cast<unsigned char>(c)) ||
                        isdigit(static_cast<unsigned char>(c));

        if (!is_valid)
            return false;
    }

    return true;
}

const std::unordered_map<std::string, NodeTag> Scanner::kKeyWords = {
    {"int", T_Int}, {"print", T_Print},
    {"if", T_If}, {"else", T_Else},
    {"while", T_While}
};

/*
 * Single character operators. '!' is listed even though it is not a token
 * on its own: it only ever introduces '!=', but the scanner still has to
 * treat it as an operator character and not glue it onto the token before
 * it. A lone '!' therefore reports itself as an unknown token.
 */
const std::unordered_map<char, NodeTag> Scanner::kSingleOp = {
    {'+', T_Plus}, {'-', T_Minus}, {'*', T_Star}, {'/', T_Slash},
    {'=', T_Assign}, {';', T_Semicolon},
    {'<', T_LT}, {'>', T_GT},
    {'{', T_LBrace}, {'}', T_RBrace},
    {'(', T_LParen}, {')', T_RParen},
    {'!', T_UnknownToken}
};

const std::unordered_map<std::string, NodeTag> Scanner::kDoubleOp = {
    {"==", T_EQ}, {"!=", T_NE},
    {"<=", T_LE}, {">=", T_GE}
};

const std::unordered_set<char> Scanner::kAlphabet = {
    '+', '-', '*', '/', '=', ';', '_', '.',
    '<', '>', '!', '{', '}', '(', ')',

    '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',

    'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j',
    'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's', 't',
    'u', 'v', 'w', 'x', 'y', 'z',

    'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J',
    'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T',
    'U', 'V', 'W', 'X', 'Y', 'Z'
};

}   /* namespace nuocc */