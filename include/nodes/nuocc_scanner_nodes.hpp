#pragma once

#include <string>

#include "nodes/nuocc_nodes.hpp"

namespace nuocc
{

template<typename T, NodeTag tag>
class Literal : public Node
{
public:
    Literal() : Node(tag) {}
    Literal(const T& value) : Node(tag), value_(value) {}

    Literal(const Literal& literal) :
        Node(literal),
        value_(literal.value_) {}
    
    ~Literal() = default;

public:
    const T& GetValue() const
    {
        return value_;
    }

private:
    T value_;
};

class KeyWord : public Node
{
public:
    KeyWord() : Node(T_KeyWord) {}
    KeyWord(const NodeTag& word) : Node(T_KeyWord), word_(word) {}

    KeyWord(const KeyWord& key_word) :
        Node(key_word),
        word_(key_word.word_) {}
    
    ~KeyWord() = default;

public:
    const NodeTag& GetWord() const
    {
        return word_;
    }

private:
    NodeTag word_;
};

class Variable : public Node
{
public:
    Variable() : Node(T_Variable) {}
    Variable(const std::string& name) :
        Node(T_Variable),
        name_(name) {}
    
    Variable(const Variable& variable) :
        Node(variable),
        name_(variable.name_) {}
    
    ~Variable() = default;

public:
    const std::string& GetName() const
    {
        return name_;
    }

private:
    std::string name_;
};

}   /* namespace nuocc */