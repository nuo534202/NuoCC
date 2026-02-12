#pragma once

#include "nodes/nodes_tag.hpp"

namespace nuocc
{

class Node
{
public:
    Node() = default;
    Node(const NodeTag& tag) : tag_(tag) {}

    Node(const Node& node)
    {
        tag_ = node.tag_;
    }

    virtual ~Node() = default;

public:
    const NodeTag& GetNodeTag() const
    {
        return tag_;
    }

    bool IsA(const NodeTag& tag) const
    {
        return tag_ == tag;
    }

private:
    NodeTag tag_;
};

}   /* namespace nuocc */