#include "nodes/nuocc_ast_nodes.hpp"

namespace nuocc
{

/* AstNode */
AstNode::AstNode(AstNodeTag node_type, PrimitiveType type)
    : left_(nullptr),
      mid_(nullptr),
      right_(nullptr),
      node_type_(node_type),
      type_(type) {}

AstNode::AstNode(AstNodeTag node_type, PrimitiveType type, AstNodePtr& left)
    : left_(std::move(left)),
      mid_(nullptr),
      right_(nullptr),
      node_type_(node_type),
      type_(type) {}

AstNode::AstNode(AstNodeTag node_type,
    PrimitiveType type,
    AstNodePtr& left,
    AstNodePtr& right)
    : left_(std::move(left)),
      mid_(nullptr),
      right_(std::move(right)),
      node_type_(node_type),
      type_(type) {}

AstNode::AstNode(AstNodeTag node_type,
    PrimitiveType type,
    AstNodePtr& left,
    AstNodePtr& mid,
    AstNodePtr& right)
    : left_(std::move(left)),
      mid_(std::move(mid)),
      right_(std::move(right)),
      node_type_(node_type),
      type_(type) {}

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

PrimitiveType AstNode::GetType() const
{
    return type_;
}

/* AstOperator */
AstOperator::AstOperator(AstNodePtr& left,
    AstNodePtr& right,
    NodeTag op_type,
    PrimitiveType type)
    : AstNode(A_AstOperator, type, left, right),
      op_type_(op_type) {}

NodeTag AstOperator::GetOpType() const
{
    return op_type_;
}

/* AstIntLit */
AstIntLit::AstIntLit(AstNodePtr& left,
    AstNodePtr& right,
    int32 value,
    PrimitiveType type)
    : AstNode(A_AstIntLit, type, left, right),
      value_(value) {}

int32 AstIntLit::GetValue() const
{
    return value_;
}

/* AstIdentifier */
AstIdentifier::AstIdentifier(AstNodePtr& left,
    AstNodePtr& right,
    const Symbol& symbol,
    bool is_lv_ident)
    : AstNode(A_AstIdentifier, symbol.type, left, right),
      symbol_(symbol),
      is_lv_ident_(is_lv_ident) {}

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
    : AstNode(A_AstPrint, PrimitiveType::kNone, expression) {}

/* AstGlue */
AstGlue::AstGlue(AstNodePtr& left, AstNodePtr& right)
    : AstNode(A_AstGlue, PrimitiveType::kNone, left, right) {}

/* AstIf */
AstIf::AstIf(AstNodePtr& condition,
    AstNodePtr& true_branch,
    AstNodePtr& false_branch,
    bool has_else)
    : AstNode(A_AstIf,
              PrimitiveType::kNone,
              condition,
              true_branch,
              false_branch),
      has_else_(has_else) {}

bool AstIf::HasElse() const
{
    return has_else_;
}

/* AstWhile */
AstWhile::AstWhile(AstNodePtr& condition, AstNodePtr& body)
    : AstNode(A_AstWhile, PrimitiveType::kNone, condition, body) {}

/* AstDeclare */
AstDeclare::AstDeclare(const Symbol& symbol)
    : AstNode(A_AstDeclare, PrimitiveType::kNone),
      symbol_(symbol) {}

const Symbol& AstDeclare::GetSymbol() const
{
    return symbol_;
}

/* AstFunction */
AstFunction::AstFunction(AstNodePtr& body, const Symbol& symbol)
    : AstNode(A_AstFunction, PrimitiveType::kNone, body),
      symbol_(symbol) {}

const Symbol& AstFunction::GetSymbol() const
{
    return symbol_;
}

/* AstWiden */
AstWiden::AstWiden(AstNodePtr& expression, PrimitiveType type)
    : AstNode(A_AstWiden, type, expression) {}

/* AstReturn */
AstReturn::AstReturn(AstNodePtr& expression)
    : AstNode(A_AstReturn, PrimitiveType::kNone, expression) {}

/* AstFuncCall */
AstFuncCall::AstFuncCall(AstNodePtr& argument, const Symbol& symbol)
    : AstNode(A_AstFuncCall, symbol.type, argument),
      symbol_(symbol) {}

const Symbol& AstFuncCall::GetSymbol() const
{
    return symbol_;
}

}   /* namespace nuocc*/
