#include "nodes/nuocc_ast_nodes.hpp"

#include "utils/nuocc_type_check.hpp"

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

AstNodePtr& AstNode::GetLeft()
{
    return left_;
}

AstNodePtr& AstNode::GetMid()
{
    return mid_;
}

AstNodePtr& AstNode::GetRight()
{
    return right_;
}

AstNodePtr AstNode::ReleaseLeft()
{
    return std::move(left_);
}

AstNodeTag AstNode::GetAstNodeTag() const
{
    return node_type_;
}

PrimitiveType AstNode::GetType() const
{
    return type_;
}

const std::shared_ptr<CompositeType>& AstNode::GetComposite() const
{
    return composite_;
}

void AstNode::SetComposite(const std::shared_ptr<CompositeType>& composite)
{
    composite_ = composite;
}

/* AstOperator */
AstOperator::AstOperator(AstNodePtr& left,
    AstNodePtr& right,
    NodeTag op_type,
    PrimitiveType type)
    : AstNode(A_AstOperator, type, left, right),
      op_type_(op_type)
{
    SetComposite(GetLeft()->GetComposite()
                     ? GetLeft()->GetComposite() : GetRight()->GetComposite());
}

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

/* AstStrLit */
AstStrLit::AstStrLit(const std::string& text)
    : AstNode(A_AstStrLit, PointerTo(PrimitiveType::kChar)),
      text_(text) {}

const std::string& AstStrLit::GetText() const
{
    return text_;
}

/* AstIdentifier */
AstIdentifier::AstIdentifier(AstNodePtr& left,
    AstNodePtr& right,
    const Symbol& symbol)
    : AstNode(A_AstIdentifier, symbol.type, left, right),
      symbol_(symbol)
{
    SetComposite(symbol.composite);
}

const Symbol& AstIdentifier::GetSymbol() const
{
    return symbol_;
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

/* AstDoWhile */
AstDoWhile::AstDoWhile(AstNodePtr& condition, AstNodePtr& body)
    : AstNode(A_AstDoWhile, PrimitiveType::kNone, condition, body) {}

/* AstFor */
AstFor::AstFor(AstNodePtr& condition, AstNodePtr& body, AstNodePtr& post)
    : AstNode(A_AstFor,
              PrimitiveType::kNone,
              condition,
              body,
              post) {}

/* AstBreak */
AstBreak::AstBreak() : AstNode(A_AstBreak, PrimitiveType::kNone) {}

/* AstContinue */
AstContinue::AstContinue() : AstNode(A_AstContinue, PrimitiveType::kNone) {}

/* AstCase */
AstCase::AstCase(AstNodePtr& body, int32 value, bool is_default)
    : AstNode(A_AstCase, PrimitiveType::kNone, body),
      value_(value),
      is_default_(is_default) {}

int32 AstCase::GetValue() const
{
    return value_;
}

bool AstCase::IsDefault() const
{
    return is_default_;
}

/* AstSwitch */
AstSwitch::AstSwitch(AstNodePtr& expression, std::vector<AstNodePtr>& cases)
    : AstNode(A_AstSwitch, PrimitiveType::kNone, expression),
      cases_(std::move(cases)) {}

const std::vector<AstNodePtr>& AstSwitch::GetCases() const
{
    return cases_;
}

std::vector<AstNodePtr>& AstSwitch::GetCases()
{
    return cases_;
}

/* AstDeclare */
AstDeclare::AstDeclare(const Symbol& symbol)
    : AstNode(A_AstDeclare, PrimitiveType::kNone),
      symbol_(symbol) {}

const Symbol& AstDeclare::GetSymbol() const
{
    return symbol_;
}

/* AstFunction */
AstFunction::AstFunction(AstNodePtr& body,
    const Symbol& symbol,
    int32 local_size,
    std::vector<Symbol>& parameters)
    : AstNode(A_AstFunction, PrimitiveType::kNone, body),
      symbol_(symbol),
      local_size_(local_size),
      parameters_(std::move(parameters)) {}

const Symbol& AstFunction::GetSymbol() const
{
    return symbol_;
}

int32 AstFunction::GetLocalSize() const
{
    return local_size_;
}

const std::vector<Symbol>& AstFunction::GetParameters() const
{
    return parameters_;
}

/* AstWiden */
AstWiden::AstWiden(AstNodePtr& expression, PrimitiveType type)
    : AstNode(A_AstWiden, type, expression) {}

/* AstScale */
AstScale::AstScale(AstNodePtr& expression, PrimitiveType type, int32 size)
    : AstNode(A_AstScale, type, expression),
      size_(size) {}

int32 AstScale::GetSize() const
{
    return size_;
}

/* AstReturn */
AstReturn::AstReturn(AstNodePtr& expression)
    : AstNode(A_AstReturn, PrimitiveType::kNone, expression) {}

/* AstFuncCall */
AstFuncCall::AstFuncCall(std::vector<AstNodePtr>& arguments,
    const Symbol& symbol)
    : AstNode(A_AstFuncCall, symbol.type),
      symbol_(symbol),
      arguments_(std::move(arguments))
{
    SetComposite(symbol.composite);
}

const Symbol& AstFuncCall::GetSymbol() const
{
    return symbol_;
}

const std::vector<AstNodePtr>& AstFuncCall::GetArguments() const
{
    return arguments_;
}

std::vector<AstNodePtr>& AstFuncCall::GetArguments()
{
    return arguments_;
}

/* AstAddress */
AstAddress::AstAddress(const Symbol& symbol)
    : AstNode(A_AstAddress, PointerTo(symbol.type)),
      symbol_(symbol)
{
    SetComposite(symbol.composite);
}

const Symbol& AstAddress::GetSymbol() const
{
    return symbol_;
}

/* AstDeref */
AstDeref::AstDeref(AstNodePtr& pointer, PrimitiveType type)
    : AstNode(A_AstDeref, type, pointer)
{
    SetComposite(GetLeft()->GetComposite());
}

/* AstMember */
AstMember::AstMember(AstNodePtr& base, const Symbol& member,
    bool through_pointer)
    : AstNode(A_AstMember,
              member.stype == StructuralType::kArray
                  ? PointerTo(member.type) : member.type,
              base),
      member_(member),
      through_pointer_(through_pointer)
{
    SetComposite(member.composite);
}

const Symbol& AstMember::GetMember() const
{
    return member_;
}

bool AstMember::IsThroughPointer() const
{
    return through_pointer_;
}

/* AstAddressExpr */
AstAddressExpr::AstAddressExpr(AstNodePtr& lvalue)
    : AstNode(A_AstAddressExpr, PointerTo(lvalue->GetType()), lvalue)
{
    SetComposite(GetLeft()->GetComposite());
}

/* AstTernary */
AstTernary::AstTernary(AstNodePtr& condition,
    AstNodePtr& when_true,
    AstNodePtr& when_false,
    PrimitiveType type,
    const std::shared_ptr<CompositeType>& composite)
    : AstNode(A_AstTernary, type, condition, when_true, when_false)
{
    SetComposite(composite);
}

/* AstCast */
AstCast::AstCast(AstNodePtr& expression, const TypeRef& type)
    : AstNode(A_AstCast, type.type, expression)
{
    SetComposite(type.composite);
}

/* AstUnary */
AstUnary::AstUnary(AstNodePtr& expression, AstNodeTag tag, PrimitiveType type)
    : AstNode(tag, type, expression) {}

/* AstIncDec */
AstIncDec::AstIncDec(const Symbol& symbol, int32 delta, bool post)
    : AstNode(A_AstIncDec, symbol.type),
      symbol_(symbol),
      delta_(delta),
      post_(post) {}

const Symbol& AstIncDec::GetSymbol() const
{
    return symbol_;
}

int32 AstIncDec::GetDelta() const
{
    return delta_;
}

bool AstIncDec::IsPost() const
{
    return post_;
}

}   /* namespace nuocc*/
