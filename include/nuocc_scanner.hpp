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

    bool IsNewToken(const std::string& token, char ch);
    void BeginToken(std::string& token, char ch);
    void AppendToken(std::string& token, char ch);
    void CommitToken(const std::string& token);

    NodeTag GetTokenNodeTag(const std::string& token);
    bool IsIntLit(const std::string& token);
    int32 ToIntLit(const std::string& token);
    bool IsIdent(const std::string& token);

private:
    std::vector<NodePtr> token_list_;
};

}   /* namespace nuocc */