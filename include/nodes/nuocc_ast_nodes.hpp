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
    AstNode(AstNodePtr& left, AstNodePtr& right);
    AstNode(AstNodePtr& left, AstNodePtr& right, AstNodeTag node_type);
    AstNode(AstNodeTag node_type);

public:
    virtual ~AstNode() = default;

public:
    void SetLeft(AstNodePtr& left);
    void SetRight(AstNodePtr& right);
    const AstNodePtr& GetLeft() const;
    const AstNodePtr& GetRight() const;
    AstNodeTag GetAstNodeTag() const;

private:
    AstNodePtr left_;
    AstNodePtr right_;
    AstNodeTag node_type_;
};

class AstOperator : public AstNode
{
public:
    AstOperator();
    AstOperator(NodeTag op_type);
    AstOperator(AstNodePtr& left, AstNodePtr& right, NodeTag op_type);
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
    AstIntLit(AstNodePtr& left, AstNodePtr& right, int32 value);
    ~AstIntLit() = default;

public:
    void SetValue(int value);
    int32 GetValue() const;

private:
    int32 value_;
};

class AstIdentifier : public AstNode
{
public:
    AstIdentifier(AstNodePtr& left,
                  AstNodePtr& right,
                  const Symbol& symbol,
                  idx_t ident_idx,
                  bool is_lv_ident);
    ~AstIdentifier() = default;

public:
    idx_t GetIdentIdx() const;
    const Symbol& GetSymbol() const;
    bool GetLvIdent() const;

private:
    Symbol symbol_;
    idx_t ident_idx_;
    bool is_lv_ident_;
};

}   /* namespace nuocc */