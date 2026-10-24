#include "nuocc_scanner.hpp"

#include <cstdlib>
#include <iostream>
#include <limits>
#include <stdexcept>

namespace nuocc
{

namespace
{

constexpr bool IsAsciiDigit(char c)
{
    return c >= '0' && c <= '9';
}

/*
 * The value of one digit of a number, or -1 when the character is not a
 * digit of any radix the language writes numbers in.
 */
int DigitValue(char c)
{
    if (IsAsciiDigit(c))
        return c - '0';

    if (c >= 'a' && c <= 'f')
        return c - 'a' + 10;

    if (c >= 'A' && c <= 'F')
        return c - 'A' + 10;

    return -1;
}

/*
 * The ASCII whitespace set is identified in one place so the token loop
 * and SkipEmpty cannot disagree about which bytes end a token.
 */
constexpr bool IsWhitespace(char c)
{
    return c == ' ' || c == '\t' || c == '\n' || c == '\r' ||
        c == '\v' || c == '\f';
}

constexpr bool IsLineBreak(char c)
{
    return c == '\n' || c == '\r';
}

constexpr bool IsAsciiAlpha(char c)
{
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
}

}   /* namespace */

/*
 * The whole program is scanned in one piece because a literal may hold a
 * space, which reading it a word at a time would split in two.
 */
void Scanner::Scan(const std::string& text)
{
    idx_t i = 0;
    StringToToken(text, i);

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

        /*
         * A line which begins with '#' is one the pre-processor wrote about
         * itself and not part of the program, so it is stepped over. It has
         * to be the first character of its line: a '#' anywhere else is a
         * character the language has no use for, and saying so is better
         * than swallowing the rest of the line and failing somewhere else.
         */
        if (buf[i] == '#' && (i == 0 || IsLineBreak(buf[i - 1])))
        {
            SkipDirective(buf, i);
            continue;
        }

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
            if (IsWhitespace(ch) || ch == '\'' || ch == '"')
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

    if (i >= buf.size() || IsLineBreak(buf[i]))
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
        if (i >= buf.size() || IsLineBreak(buf[i]))
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
 * Read one character of a literal, interpret a backslash escape, and leave
 * i just after what was read. A character may be written as itself, as one
 * of the simple escapes, or by its code in octal or in hexadecimal.
 */
char Scanner::ScanEscape(const std::string& buf, idx_t& i)
{
    char c = buf[i++];

    if (c != '\\')
        return c;

    if (i >= buf.size() || IsLineBreak(buf[i]))
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
        /*
         * The code of a character may be written instead of the character
         * itself: in octal, as up to three digits, or in hexadecimal after
         * an 'x'. The first digit of an octal code has been read already as
         * the escape character, so it is given back for the code to be read
         * from its beginning.
         */
        case '0': case '1': case '2': case '3':
        case '4': case '5': case '6': case '7':
            i--;
            return static_cast<char>(ScanCode(buf, i, 8));
        case 'x':
            return static_cast<char>(ScanCode(buf, i, 16));
        default:
            std::cerr << "lexical error: unknown escape sequence \\";
            std::cerr << escaped << "!" << std::endl;
            std::exit(1);
    }
}

/*
 * The character code an escape names, which begins at buf[i] and is written
 * in the radix given: an octal code is up to three digits long, while a
 * hexadecimal one runs for as many digits as are written. i is left at the
 * first character which is not a digit of the code, which is the one after
 * the escape and is read again by whoever asked for it.
 */
int Scanner::ScanCode(const std::string& buf, idx_t& i, int radix)
{
    /* A radix of zero digits means as many digits as are written. */
    const int most = radix == 16 ? 0 : 3;

    int64 value = 0;
    int digits = 0;

    while (i < buf.size() && (most == 0 || digits < most))
    {
        const int digit = DigitValue(buf[i]);

        if (digit < 0 || digit >= radix)
            break;

        value = value * radix + digit;
        digits++;
        i++;

        /* A character is one byte, so a code which is larger than one does
         * not name a character at all. */
        if (value > 255)
        {
            std::cerr << "lexical error: the character code " << value;
            std::cerr << " is too large for a char!" << std::endl;
            std::exit(1);
        }
    }

    if (digits == 0)
    {
        std::cerr << "lexical error: there are no digits in the character";
        std::cerr << " code!" << std::endl;
        std::exit(1);
    }

    return static_cast<int>(value);
}

void Scanner::SkipEmpty(const std::string& buf, idx_t& i)
{
    while (i < buf.size() && IsWhitespace(buf[i]))
        i++;
}

void Scanner::SkipDirective(const std::string& buf, idx_t& i)
{
    while (i < buf.size() && !IsLineBreak(buf[i]))
        i++;
}

bool Scanner::BeginsOperator(const std::string& text)
{
    if (text.empty())
        return false;

    for (const std::pair<const std::string, NodeTag>& op : kDoubleOp)
    {
        if (op.first.size() >= text.size() &&
            op.first.compare(0, text.size(), text) == 0)
            return true;
    }

    for (const std::pair<const std::string, NodeTag>& op : kTripleOp)
    {
        if (op.first.size() >= text.size() &&
            op.first.compare(0, text.size(), text) == 0)
            return true;
    }

    return false;
}

bool Scanner::IsNewToken(const std::string& token, char ch)
{
    /*
     * The next character is taken into the token when the two of them
     * together could still grow into an operator, as '=' and '=' grow into
     * '=='. One '.' and two of them can grow into the '...' of a parameter
     * list and are not operators on their own, which is what this catches.
     */
    if (BeginsOperator(token + ch))
        return false;

    /* An operator is at most three characters long, so it ends here. */
    if (kDoubleOp.find(token) != kDoubleOp.end() ||
        kTripleOp.find(token) != kTripleOp.end())
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
        case T_Percent:
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
        case T_Dot:
        case T_Arrow:
        case T_LShift:
        case T_RShift:
        case T_Inc:
        case T_Dec:
        case T_Amper:
        case T_Comma:
        case T_Colon:
        case T_LogAnd:
        case T_LogOr:
        case T_Or:
        case T_Xor:
        case T_Invert:
        case T_LogNot:
        case T_PlusAssign:
        case T_MinusAssign:
        case T_StarAssign:
        case T_SlashAssign:
        case T_PercentAssign:
        case T_AmperAssign:
        case T_OrAssign:
        case T_XorAssign:
        case T_LShiftAssign:
        case T_RShiftAssign:
        case T_Question:
        case T_Ellipsis:
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

    if (token.size() == 3 && kTripleOp.find(token) != kTripleOp.end())
        return kTripleOp.at(token);

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
    /*
     * A word which begins with a digit is a number: which digits it may be
     * written with is decided by the radix it says it is written in, and
     * ToIntLit is what reads it and reports a digit of the wrong radix.
     */
    return !token.empty() && IsAsciiDigit(token.front());
}

/*
 * Convert a token made of digits into its value. The language has no
 * integer type wider than int yet, so a literal which does not fit is an
 * error rather than a silent overflow.
 */
int32 Scanner::ToIntLit(const std::string& token)
{
    /*
     * A number which begins with a zero is written in another radix: `0x`
     * begins a hexadecimal number and a zero on its own begins an octal
     * one.
     */
    int radix = 10;
    idx_t start = 0;

    if (token.front() == '0' && token.size() > 1)
    {
        radix = 8;

        if (token[1] == 'x' || token[1] == 'X')
        {
            radix = 16;
            start = 2;
        }
    }

    if (start == token.size())
    {
        std::cerr << "lexical error: there are no digits after the 0x of ";
        std::cerr << token << "!" << std::endl;
        std::exit(1);
    }

    int64 value = 0;

    for (idx_t k = start; k < token.size(); k++)
    {
        const int digit = DigitValue(token[k]);

        if (digit < 0 || digit >= radix)
        {
            std::cerr << "lexical error: the digit " << token[k] << " of ";
            std::cerr << token << " is not one a number of radix " << radix;
            std::cerr << " holds!" << std::endl;
            std::exit(1);
        }

        value = value * radix + digit;

        /* A number the language holds is one of four bytes, and one too
         * large to be held is refused rather than wrapped round. */
        if (value > std::numeric_limits<int32>::max())
        {
            std::cerr << "lexical error: integer literal " << token;
            std::cerr << " is too large!" << std::endl;
            std::exit(1);
        }
    }

    return static_cast<int32>(value);
}

bool Scanner::IsIdent(const std::string& token)
{
    if (token.empty())
        return false;

    if (IsAsciiDigit(token.front()))
        return false;

    for (char c : token)
    {
        const bool is_valid = c == '_' || IsAsciiAlpha(c) ||
            IsAsciiDigit(c);

        if (!is_valid)
            return false;
    }

    return true;
}

const std::unordered_map<std::string, NodeTag> Scanner::kKeyWords = {
    {"int", T_Int}, {"char", T_Char}, {"short", T_Short}, {"long", T_Long},
    {"print", T_Print},
    {"struct", T_Struct}, {"union", T_Union},
    {"if", T_If}, {"else", T_Else},
    {"while", T_While}, {"do", T_Do}, {"for", T_For},
    {"void", T_Void}, {"return", T_Return},
    {"enum", T_Enum}, {"typedef", T_Typedef},
    {"extern", T_Extern},
    {"break", T_Break}, {"continue", T_Continue},
    {"switch", T_Switch}, {"case", T_Case}, {"default", T_Default},
    {"sizeof", T_Sizeof},
    {"static", T_Static},
    {"goto", T_Goto}
};

/*
 * Single character operators. Some of these characters also begin a two
 * character operator, which kDoubleOp below is asked about first.
 */
const std::unordered_map<char, NodeTag> Scanner::kSingleOp = {
    {'+', T_Plus}, {'-', T_Minus}, {'*', T_Star}, {'/', T_Slash},
    {'%', T_Percent},
    {'=', T_Assign}, {';', T_Semicolon}, {',', T_Comma}, {':', T_Colon},
    {'<', T_LT}, {'>', T_GT},
    {'{', T_LBrace}, {'}', T_RBrace},
    {'(', T_LParen}, {')', T_RParen},
    {'[', T_LBracket}, {']', T_RBracket}, {'.', T_Dot},
    {'&', T_Amper}, {'|', T_Or}, {'^', T_Xor},
    {'~', T_Invert}, {'!', T_LogNot}, {'?', T_Question}
};

/*
 * The operators written with three characters. They are asked about before
 * the two character ones, since the two characters a three character
 * operator begins with may be an operator of their own, as `<<` is, and
 * the longer one is the one meant.
 */
const std::unordered_map<std::string, NodeTag> Scanner::kTripleOp = {
    {"...", T_Ellipsis},
    {"<<=", T_LShiftAssign}, {">>=", T_RShiftAssign}
};

const std::unordered_map<std::string, NodeTag> Scanner::kDoubleOp = {
    {"==", T_EQ}, {"!=", T_NE},
    {"<=", T_LE}, {">=", T_GE}, {"<<", T_LShift}, {">>", T_RShift},
    {"&&", T_LogAnd}, {"||", T_LogOr},
    {"++", T_Inc}, {"--", T_Dec}, {"->", T_Arrow},
    {"+=", T_PlusAssign}, {"-=", T_MinusAssign},
    {"*=", T_StarAssign}, {"/=", T_SlashAssign},
    {"%=", T_PercentAssign},
    {"&=", T_AmperAssign}, {"|=", T_OrAssign}, {"^=", T_XorAssign}
};

const std::unordered_set<char> Scanner::kAlphabet = {
    '+', '-', '*', '/', '%', '=', ';', ',', ':', '_', '.',
    '<', '>', '!', '{', '}', '(', ')', '[', ']', '&', '|', '^', '~', '?',

    '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',

    'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j',
    'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's', 't',
    'u', 'v', 'w', 'x', 'y', 'z',

    'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J',
    'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T',
    'U', 'V', 'W', 'X', 'Y', 'Z'
};

}   /* namespace nuocc */
