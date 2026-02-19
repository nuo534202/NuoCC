#include "nodes/nuocc_ast_nodes.hpp"

namespace nuocc
{

/* AstNode */
AstNode::AstNode(std::unique_ptr<AstNode>& left,
                 std::unique_ptr<AstNode>& right)
    : left_(std::move(left)),
      right_(std::move(right)),
      node_type_(A_AstNode) {}

AstNode::AstNode(std::unique_ptr<AstNode>& left,
                 std::unique_ptr<AstNode>& right,
                 AstNodeTag node_type)
    : left_(std::move(left)),
      right_(std::move(right)),
      node_type_(node_type) {}

AstNode::AstNode(AstNodeTag node_type)
    : left_(nullptr),
      right_(nullptr),
      node_type_(node_type) {}

void AstNode::SetLeft(std::unique_ptr<AstNode>& left)
{
    left_ = std::move(left);
}

void AstNode::SetRight(std::unique_ptr<AstNode>& right)
{
    right_ = std::move(right);
}

const std::unique_ptr<AstNode>& AstNode::GetLeft() const
{
    return left_;
}

const std::unique_ptr<AstNode>& AstNode::GetRight() const
{
    return right_;
}

AstNodeTag AstNode::GetAstNodeTag() const
{
    return node_type_;
}

/* AstOperator */
AstOperator::AstOperator()
    : AstNode(A_AstOperator) {}

AstOperator::AstOperator(NodeTag op_type)
    : AstNode(A_AstOperator), op_type_(op_type) {}

AstOperator::AstOperator(std::unique_ptr<AstNode>& left,
                         std::unique_ptr<AstNode>& right,
                         NodeTag op_type)
    : AstNode(left, right, A_AstOperator),
      op_type_(op_type) {}

NodeTag AstOperator::GetOpType() const
{
    return op_type_;
}

/* AstIntLit */
AstIntLit::AstIntLit() : AstNode(A_AstIntLit) {}

AstIntLit::AstIntLit(int32 value)
    : AstNode(A_AstIntLit),
      value_(value) {}

AstIntLit::AstIntLit(std::unique_ptr<AstNode>& left,
                     std::unique_ptr<AstNode>& right,
                     int32 value)
    : AstNode(left, right, A_AstIntLit),
      value_(value) {}

void AstIntLit::SetValue(int value)
{
    value_ = value;
}

int32 AstIntLit::GetValue() const
{
    return value_;
}

}   /* namespace nuocc*/