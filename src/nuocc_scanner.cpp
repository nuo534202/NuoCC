#include "nuocc_scanner.hpp"

#include <cctype>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <limits>
#include <stdexcept>

namespace nuocc
{

/*
 * The whole file is read in at once because a literal may hold a space,
 * which the word at a time reading below would split in two.
 */
void Scanner::Scan(const std::string& file)
{
    std::ifstream ifs;

    ifs.open(file.c_str(), std::ios::in);
    if (!ifs.is_open())
    {
        std::cerr << "Error: Fail to open file " << file << std::endl;
        std::exit(1);
    }

    std::string source;
    std::string line;

    while (std::getline(ifs, line))
        source += line + '\n';

    ifs.close();

    idx_t i = 0;
    StringToToken(source, i);

    token_list_.push_back(std::make_unique<Node>(T_EOF));
}

const std::vector<NodePtr>& Scanner::GetTokenList() const
{
    return token_list_;
}

void Scanner::StringToToken(const std::string& buf, idx_t& i)
{
    while (true)
    {
        SkipEmpty(buf, i);

        if (i >= buf.size())
            return;

        if (buf[i] == '\'')
        {
            ScanCharLiteral(buf, i);
            continue;
        }

        if (buf[i] == '"')
        {
            ScanStrLiteral(buf, i);
            continue;
        }

        std::string token;

        for (; i < buf.size(); i++)
        {
            char ch = buf[i];

            /*
             * A word ends at whitespace and at the quote which starts a
             * literal, so the loop below can pick the literal up again.
             */
            if (std::isspace(static_cast<unsigned char>(ch)) ||
                ch == '\'' || ch == '"')
                break;

            /* character is not in the alphabet */
            if (kAlphabet.find(ch) == kAlphabet.end())
            {
                std::cerr << "Error: unrecognized character " << ch;
                std::cerr << "!" << std::endl;
                std::exit(1);
            }

            if (IsNewToken(token, ch))
            {
                CommitToken(token);
                BeginToken(token, ch);
                continue;
            }

            AppendToken(token, ch);
        }

        CommitToken(token);
    }
}

/*
 * A character literal holds one character between two single quotes and
 * scans as an integer literal: its value is the character's code, and the
 * parser gives a small literal the type char.
 */
void Scanner::ScanCharLiteral(const std::string& buf, idx_t& i)
{
    i++;    /* step over the opening quote */

    if (i >= buf.size() || buf[i] == '\n')
    {
        std::cerr << "lexical error: unterminated character literal!";
        std::cerr << std::endl;
        std::exit(1);
    }

    char value = ScanEscape(buf, i);

    if (i >= buf.size() || buf[i] != '\'')
    {
        std::cerr << "lexical error: expected ' at end of character ";
        std::cerr << "literal!" << std::endl;
        std::exit(1);
    }

    i++;    /* step over the closing quote */

    token_list_.push_back(std::make_unique<Literal<int, T_IntLit>>(
        static_cast<unsigned char>(value)));
}

/*
 * A string literal holds zero or more characters between two double
 * quotes and scans into a token of its own, whose text stands for the
 * string. The storage for the characters comes later, when the string is
 * used, so only the text is kept here.
 */
void Scanner::ScanStrLiteral(const std::string& buf, idx_t& i)
{
    i++;    /* step over the opening quote */

    std::string text;

    while (true)
    {
        /* A string may not reach the end of the file or a new line. */
        if (i >= buf.size() || buf[i] == '\n')
        {
            std::cerr << "lexical error: unterminated string literal!";
            std::cerr << std::endl;
            std::exit(1);
        }

        if (buf[i] == '"')
        {
            i++;    /* step over the closing quote */
            break;
        }

        text.push_back(ScanEscape(buf, i));
    }

    token_list_.push_back(
        std::make_unique<Literal<std::string, T_StrLit>>(text));
}

/*
 * Read one character of a literal, interpret a backslash escape, and
 * leave i just after what was read. Only the simple escapes are known:
 * an octal code or a Unicode value is an error rather than a guess.
 */
char Scanner::ScanEscape(const std::string& buf, idx_t& i)
{
    char c = buf[i++];

    if (c != '\\')
        return c;

    if (i >= buf.size() || buf[i] == '\n')
    {
        std::cerr << "lexical error: expected a character after a ";
        std::cerr << "backslash!" << std::endl;
        std::exit(1);
    }

    char escaped = buf[i++];

    switch (escaped)
    {
        case 'a':  return '\a';
        case 'b':  return '\b';
        case 'f':  return '\f';
        case 'n':  return '\n';
        case 'r':  return '\r';
        case 't':  return '\t';
        case 'v':  return '\v';
        case '\\': return '\\';
        case '"':  return '"';
        case '\'': return '\'';
        default:
            std::cerr << "lexical error: unknown escape sequence \\";
            std::cerr << escaped << "!" << std::endl;
            std::exit(1);
    }
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
        case T_LBracket:
        case T_RBracket:
        case T_LShift:
        case T_RShift:
        case T_Inc:
        case T_Dec:
        case T_Amper:
        case T_Comma:
        case T_LogAnd:
        case T_LogOr:
        case T_Or:
        case T_Xor:
        case T_Invert:
        case T_LogNot:
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
    {"int", T_Int}, {"char", T_Char}, {"long", T_Long}, {"print", T_Print},
    {"if", T_If}, {"else", T_Else},
    {"while", T_While}, {"for", T_For},
    {"void", T_Void}, {"return", T_Return}
};

/*
 * Single character operators. Some of these characters also begin a two
 * character operator, which kDoubleOp below is asked about first.
 */
const std::unordered_map<char, NodeTag> Scanner::kSingleOp = {
    {'+', T_Plus}, {'-', T_Minus}, {'*', T_Star}, {'/', T_Slash},
    {'=', T_Assign}, {';', T_Semicolon}, {',', T_Comma},
    {'<', T_LT}, {'>', T_GT},
    {'{', T_LBrace}, {'}', T_RBrace},
    {'(', T_LParen}, {')', T_RParen},
    {'[', T_LBracket}, {']', T_RBracket},
    {'&', T_Amper}, {'|', T_Or}, {'^', T_Xor},
    {'~', T_Invert}, {'!', T_LogNot}
};

const std::unordered_map<std::string, NodeTag> Scanner::kDoubleOp = {
    {"==", T_EQ}, {"!=", T_NE},
    {"<=", T_LE}, {">=", T_GE}, {"<<", T_LShift}, {">>", T_RShift},
    {"&&", T_LogAnd}, {"||", T_LogOr},
    {"++", T_Inc}, {"--", T_Dec}
};

const std::unordered_set<char> Scanner::kAlphabet = {
    '+', '-', '*', '/', '=', ';', ',', '_', '.',
    '<', '>', '!', '{', '}', '(', ')', '[', ']', '&', '|', '^', '~',

    '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',

    'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j',
    'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's', 't',
    'u', 'v', 'w', 'x', 'y', 'z',

    'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J',
    'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T',
    'U', 'V', 'W', 'X', 'Y', 'Z'
};

}   /* namespace nuocc */