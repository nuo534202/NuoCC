#pragma once

#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "nodes/nodes.hpp"
#include "nodes/scanner_nodes.hpp"

namespace nuocc
{

class Scanner
{
private:
    static const std::unordered_map<std::string, NodeTag> key_words;
    static const std::unordered_map<char, NodeTag> single_op;
    static const std::unordered_set<char> alphabet;

public:
    Scanner() = default;
    Scanner(const std::string& filename);
    ~Scanner() = default;

public:
    void Scan();
    const std::string& GetFileName() const;
    const std::vector<std::unique_ptr<Node>>& GetTokenList() const;

private:
    void SkipEmpty(const std::string& buf, size_t& i);

    void StringToToken(const std::string& buf, size_t& i);

    bool IsNewToken(const std::string& token, char ch);
    void BeginToken(std::string& token, char ch);
    void AppendToken(std::string& token, char ch);
    void CommitToken(const std::string& token);

    NodeTag GetTokenNodeTag(const std::string& token);
    bool IsIntLit(const std::string& token);
    bool IsVariable(const std::string& token);

private:
    std::string file_name_;
    std::vector<std::unique_ptr<Node>> token_list_;
};

}   /* namespace nuocc */