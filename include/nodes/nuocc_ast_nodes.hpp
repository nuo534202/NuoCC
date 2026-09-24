#pragma once

#include <memory>

#include "nodes/nuocc_nodes_tag.hpp"
#include "utils/nuocc_types.hpp"

namespace nuocc
{

/*
 * Every node of the abstract syntax tree. A node holds up to three
 * children: an if statement uses all of them (the condition, the true
 * branch and the optional else branch), a glue node uses the left and
 * the right child, and an operator or a print node only uses the left
 * ones. An unused child stays a null pointer.
 */
class AstNode
{
protected:
    AstNode() = default;
    AstNode(AstNodeTag node_type);
    AstNode(AstNodeTag node_type, AstNodePtr& left);
    AstNode(AstNodeTag node_type, AstNodePtr& left, AstNodePtr& right);
    AstNode(AstNodeTag node_type,
            AstNodePtr& left,
            AstNodePtr& mid,
            AstNodePtr& right);

public:
    virtual ~AstNode() = default;

public:
    void SetLeft(AstNodePtr& left);
    void SetMid(AstNodePtr& mid);
    void SetRight(AstNodePtr& right);

    const AstNodePtr& GetLeft() const;
    const AstNodePtr& GetMid() const;
    const AstNodePtr& GetRight() const;

    AstNodeTag GetAstNodeTag() const;

private:
    AstNodePtr left_;
    AstNodePtr mid_;
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

/*
 * A statement which prints the value of its left child followed by a
 * newline.
 */
class AstPrint : public AstNode
{
public:
    explicit AstPrint(AstNodePtr& expression);
    ~AstPrint() = default;
};

/*
 * Two statements glued together. They are executed left to right, which
 * is how a compound statement becomes a single tree.
 */
class AstGlue : public AstNode
{
public:
    AstGlue(AstNodePtr& left, AstNodePtr& right);
    ~AstGlue() = default;
};

/*
 * An if statement holding the condition in the left child, the compound
 * statement following the condition in the middle child and the optional
 * compound statement after 'else' in the right child. A branch is null
 * when its compound statement is empty, so has_else records whether the
 * source had an else clause at all.
 */
class AstIf : public AstNode
{
public:
    AstIf(AstNodePtr& condition,
          AstNodePtr& true_branch,
          AstNodePtr& false_branch,
          bool has_else);
    ~AstIf() = default;

public:
    bool HasElse() const;

private:
    bool has_else_;
};

/*
 * The declaration of a global variable.
 */
class AstDeclare : public AstNode
{
public:
    explicit AstDeclare(const Symbol& symbol);
    ~AstDeclare() = default;

public:
    const Symbol& GetSymbol() const;

private:
    Symbol symbol_;
};

}   /* namespace nuocc */
