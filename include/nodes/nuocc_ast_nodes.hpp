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
 *
 * A node which stands for an expression also carries the primitive type
 * of the value it produces. Statement nodes carry kNone.
 */
class AstNode
{
protected:
    AstNode() = default;
    AstNode(AstNodeTag node_type, PrimitiveType type);
    AstNode(AstNodeTag node_type, PrimitiveType type, AstNodePtr& left);
    AstNode(AstNodeTag node_type,
            PrimitiveType type,
            AstNodePtr& left,
            AstNodePtr& right);
    AstNode(AstNodeTag node_type,
            PrimitiveType type,
            AstNodePtr& left,
            AstNodePtr& mid,
            AstNodePtr& right);

public:
    virtual ~AstNode() = default;

public:
    const AstNodePtr& GetLeft() const;
    const AstNodePtr& GetMid() const;
    const AstNodePtr& GetRight() const;

    AstNodeTag GetAstNodeTag() const;
    PrimitiveType GetType() const;

private:
    AstNodePtr left_;
    AstNodePtr mid_;
    AstNodePtr right_;
    AstNodeTag node_type_;
    PrimitiveType type_;
};

/* A binary operator, holding the type of the value it produces. */
class AstOperator : public AstNode
{
public:
    AstOperator(AstNodePtr& left,
                AstNodePtr& right,
                NodeTag op_type,
                PrimitiveType type);
    ~AstOperator() = default;

public:
    NodeTag GetOpType() const;

private:
    NodeTag op_type_;
};

/* An integer literal, whose type records whether it fits a char. */
class AstIntLit : public AstNode
{
public:
    AstIntLit(AstNodePtr& left,
              AstNodePtr& right,
              int32 value,
              PrimitiveType type);
    ~AstIntLit() = default;

public:
    int32 GetValue() const;

private:
    int32 value_;
};

/* A reference to a variable, either as its value or as an lvalue target. */
class AstIdentifier : public AstNode
{
public:
    AstIdentifier(AstNodePtr& left,
                  AstNodePtr& right,
                  const Symbol& symbol,
                  bool is_lv_ident);
    ~AstIdentifier() = default;

public:
    const Symbol& GetSymbol() const;
    bool GetLvIdent() const;

private:
    Symbol symbol_;
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
 * A while loop holding the condition in the left child and the compound
 * statement which is the body of the loop in the right child.
 */
class AstWhile : public AstNode
{
public:
    AstWhile(AstNodePtr& condition, AstNodePtr& body);
    ~AstWhile() = default;
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

/*
 * A function declaration. All functions are void and take no arguments for
 * now, so the left child holding the body is all there is besides the name.
 */
class AstFunction : public AstNode
{
public:
    AstFunction(AstNodePtr& body, const Symbol& symbol);
    ~AstFunction() = default;

public:
    const Symbol& GetSymbol() const;

private:
    Symbol symbol_;
};

/*
 * Widen the value of the left child, which is narrower than the type of
 * this node, so that the value can be used at the wider type.
 */
class AstWiden : public AstNode
{
public:
    AstWiden(AstNodePtr& expression, PrimitiveType type);
    ~AstWiden() = default;
};

}   /* namespace nuocc */
