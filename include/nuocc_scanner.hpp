#pragma once

#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "nodes/nuocc_nodes.hpp"
#include "nodes/nuocc_scanner_nodes.hpp"
#include "utils/nuocc_types.hpp"

namespace nuocc
{

class Scanner
{
private:
    static const std::unordered_map<std::string, NodeTag> kKeyWords;
    static const std::unordered_map<char, NodeTag> kSingleOp;
    static const std::unordered_map<std::string, NodeTag> kDoubleOp;
    static const std::unordered_set<char> kAlphabet;

public:
    Scanner() = default;
    ~Scanner() = default;

public:
    /*
     * Scan the whole program, which is the text the pre-processor wrote
     * for one source file: the comments, the #include lines and the
     * pre-processor's own markers are all handled here.
     */
    void Scan(const std::string& text);
    const std::vector<NodePtr>& GetTokenList() const;

private:
    void StringToToken(const std::string& buf, idx_t& i);
    void SkipEmpty(const std::string& buf, idx_t& i);

    /*
     * Step over one line the pre-processor wrote to say which file and
     * line the text after it came from. Such a line starts with '#', which
     * is no character of the language and is never part of a program.
     */
    void SkipDirective(const std::string& buf, idx_t& i);

    void ScanCharLiteral(const std::string& buf, idx_t& i);
    void ScanStrLiteral(const std::string& buf, idx_t& i);
    char ScanEscape(const std::string& buf, idx_t& i);

    /*
     * The character code an escape names, which begins at buf[i] and is
     * written in the radix given. i is left at the first character which is
     * not a digit of the code.
     */
    int ScanCode(const std::string& buf, idx_t& i, int radix);

    bool IsNewToken(const std::string& token, char ch);
    void BeginToken(std::string& token, char ch);
    void AppendToken(std::string& token, char ch);
    void CommitToken(const std::string& token);

    NodeTag GetTokenNodeTag(const std::string& token);

    /*
     * Whether a word is a number rather than a name. Everything which
     * begins with a digit is one, and which digits it may be written with
     * is worked out when it is read, so that a digit of the wrong radix is
     * reported as such rather than left as an unknown token.
     */
    bool IsIntLit(const std::string& token);

    /*
     * The number a word stands for, which is read in the radix the word
     * says it is written in: `123` and `0x7b` are both 123, and so is
     * `0173`, since a number which begins with a zero is written in
     * octal. A digit which that radix has no room for, and a number too
     * large to be held, stop the compilation here.
     */
    int32 ToIntLit(const std::string& token);
    bool IsIdent(const std::string& token);

private:
    std::vector<NodePtr> token_list_;
};

}   /* namespace nuocc */