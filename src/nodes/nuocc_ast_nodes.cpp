#include "nodes/nuocc_ast_nodes.hpp"

namespace nuocc
{

/* AstNode */
AstNode::AstNode(AstNodePtr& left, AstNodePtr& right)
    : left_(std::move(left)),
      right_(std::move(right)),
      node_type_(A_AstNode) {}

AstNode::AstNode(AstNodePtr& left, AstNodePtr& right, AstNodeTag node_type)
    : left_(std::move(left)),
      right_(std::move(right)),
      node_type_(node_type) {}

AstNode::AstNode(AstNodeTag node_type)
    : left_(nullptr),
      right_(nullptr),
      node_type_(node_type) {}

void AstNode::SetLeft(AstNodePtr& left)
{
    left_ = std::move(left);
}

void AstNode::SetRight(AstNodePtr& right)
{
    right_ = std::move(right);
}

const AstNodePtr& AstNode::GetLeft() const
{
    return left_;
}

const AstNodePtr& AstNode::GetRight() const
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

AstOperator::AstOperator(AstNodePtr& left, AstNodePtr& right, NodeTag op_type)
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

AstIntLit::AstIntLit(AstNodePtr& left, AstNodePtr& right, int32 value)
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

/* AstIdentifier */
AstIdentifier::AstIdentifier(AstNodePtr& left,
    AstNodePtr& right,
    const Symbol& symbol,
    idx_t ident_idx,
    bool is_lv_ident)
    : AstNode(left, right, A_AstIdentifier),
      symbol_(symbol),
      ident_idx_(ident_idx) {}

idx_t AstIdentifier::GetIdentIdx() const
{
    return ident_idx_;
}

const Symbol& AstIdentifier::GetSymbol() const
{
    return symbol_;
}

bool AstIdentifier::GetLvIdent() const
{
    return is_lv_ident_;
}


}   /* namespace nuocc*/