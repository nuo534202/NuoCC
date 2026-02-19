#pragma once

#include <memory>

#include "nodes/nuocc_nodes_tag.hpp"
#include "utils/nuocc_types.hpp"

namespace nuocc
{

class AstNode
{
protected:
    AstNode() = default;
    AstNode(std::unique_ptr<AstNode>& left,
            std::unique_ptr<AstNode>& right);
    AstNode(std::unique_ptr<AstNode>& left,
            std::unique_ptr<AstNode>& right,
            AstNodeTag node_type);
    AstNode(AstNodeTag node_type);

public:
    virtual ~AstNode() = default;

public:
    void SetLeft(std::unique_ptr<AstNode>& left);
    void SetRight(std::unique_ptr<AstNode>& right);
    const std::unique_ptr<AstNode>& GetLeft() const;
    const std::unique_ptr<AstNode>& GetRight() const;
    AstNodeTag GetAstNodeTag() const;

private:
    std::unique_ptr<AstNode> left_;
    std::unique_ptr<AstNode> right_;
    AstNodeTag node_type_;
};

class AstOperator : public AstNode
{
public:
    AstOperator();
    AstOperator(NodeTag op_type);
    AstOperator(std::unique_ptr<AstNode>& left,
                std::unique_ptr<AstNode>& right,
                NodeTag op_type);
    ~AstOperator() = default;

public:
    NodeTag GetOpType() const;

private:
    NodeTag op_type_;
};

class AstIntLit : public AstNode
{
public:
    AstIntLit();
    AstIntLit(int32 value);
    AstIntLit(std::unique_ptr<AstNode>& left,
              std::unique_ptr<AstNode>& right,
              int32 value);
    ~AstIntLit() = default;

public:
    void SetValue(int value);
    int32 GetValue() const;

private:
    int32 value_;
};

}   /* namespace nuocc */