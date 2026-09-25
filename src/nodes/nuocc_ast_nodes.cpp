#include "nodes/nuocc_ast_nodes.hpp"

namespace nuocc
{

/* AstNode */
AstNode::AstNode(AstNodeTag node_type)
    : left_(nullptr),
      mid_(nullptr),
      right_(nullptr),
      node_type_(node_type) {}

AstNode::AstNode(AstNodeTag node_type, AstNodePtr& left)
    : left_(std::move(left)),
      mid_(nullptr),
      right_(nullptr),
      node_type_(node_type) {}

AstNode::AstNode(AstNodeTag node_type, AstNodePtr& left, AstNodePtr& right)
    : left_(std::move(left)),
      mid_(nullptr),
      right_(std::move(right)),
      node_type_(node_type) {}

AstNode::AstNode(AstNodeTag node_type,
                 AstNodePtr& left,
                 AstNodePtr& mid,
                 AstNodePtr& right)
    : left_(std::move(left)),
      mid_(std::move(mid)),
      right_(std::move(right)),
      node_type_(node_type) {}

void AstNode::SetLeft(AstNodePtr& left)
{
    left_ = std::move(left);
}

void AstNode::SetMid(AstNodePtr& mid)
{
    mid_ = std::move(mid);
}

void AstNode::SetRight(AstNodePtr& right)
{
    right_ = std::move(right);
}

const AstNodePtr& AstNode::GetLeft() const
{
    return left_;
}

const AstNodePtr& AstNode::GetMid() const
{
    return mid_;
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
    : AstNode(A_AstOperator, left, right),
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
    : AstNode(A_AstIntLit, left, right),
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
    : AstNode(A_AstIdentifier, left, right),
      symbol_(symbol),
      ident_idx_(ident_idx),
      is_lv_ident_(is_lv_ident) {}

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

/* AstPrint */
AstPrint::AstPrint(AstNodePtr& expression)
    : AstNode(A_AstPrint, expression) {}

/* AstGlue */
AstGlue::AstGlue(AstNodePtr& left, AstNodePtr& right)
    : AstNode(A_AstGlue, left, right) {}

/* AstIf */
AstIf::AstIf(AstNodePtr& condition,
    AstNodePtr& true_branch,
    AstNodePtr& false_branch,
    bool has_else)
    : AstNode(A_AstIf, condition, true_branch, false_branch),
      has_else_(has_else) {}

bool AstIf::HasElse() const
{
    return has_else_;
}

/* AstWhile */
AstWhile::AstWhile(AstNodePtr& condition, AstNodePtr& body)
    : AstNode(A_AstWhile, condition, body) {}

/* AstDeclare */
AstDeclare::AstDeclare(const Symbol& symbol)
    : AstNode(A_AstDeclare),
      symbol_(symbol) {}

const Symbol& AstDeclare::GetSymbol() const
{
    return symbol_;
}

}   /* namespace nuocc*/
