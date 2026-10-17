#pragma once

#include <memory>
#include <string>
#include <vector>

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

    /*
     * The same children of a node which is not const. A tree which is being
     * worked on rather than read, such as one an optimisation is folding,
     * takes a child this way so that a smaller tree can be left in its
     * place.
     */
    AstNodePtr& GetLeft();
    AstNodePtr& GetMid();
    AstNodePtr& GetRight();

    /*
     * Move a child out of this node, which is left without one. It is how
     * '&' of a dereference takes back the expression the '*' was written
     * before, as '&*p' is p itself, and how an operation which does nothing
     * with one of its sides leaves the other one in its place.
     */
    AstNodePtr ReleaseLeft();
    AstNodePtr ReleaseRight();

    AstNodeTag GetAstNodeTag() const;
    PrimitiveType GetType() const;
    const std::shared_ptr<CompositeType>& GetComposite() const;

protected:
    void SetComposite(const std::shared_ptr<CompositeType>& composite);

private:
    AstNodePtr left_;
    AstNodePtr mid_;
    AstNodePtr right_;
    AstNodeTag node_type_;
    PrimitiveType type_;
    std::shared_ptr<CompositeType> composite_;
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

/*
 * A string literal, always of type char *. It holds the characters of
 * one "..." literal; the static storage for them is created when the
 * literal is used.
 */
class AstStrLit : public AstNode
{
public:
    AstStrLit(const std::string& text);
    ~AstStrLit() = default;

public:
    const std::string& GetText() const;

private:
    std::string text_;
};

/*
 * A reference to a variable. Reading one loads the value it holds; a
 * variable written to is not read at all, as the assignment which stores
 * into it reaches it by name and not through a register.
 */
class AstIdentifier : public AstNode
{
public:
    AstIdentifier(AstNodePtr& left,
                  AstNodePtr& right,
                  const Symbol& symbol);
    ~AstIdentifier() = default;

public:
    const Symbol& GetSymbol() const;

private:
    Symbol symbol_;
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
 * A do loop, which is a while loop with the condition at the end: the
 * body runs once before the condition is asked at all, and every pass
 * after that begins only where it held. The left child holds the
 * condition and the right child the body, in the same places a while loop
 * keeps them, so that everything which walks a tree reaches both.
 */
class AstDoWhile : public AstNode
{
public:
    AstDoWhile(AstNodePtr& condition, AstNodePtr& body);
    ~AstDoWhile() = default;
};

/*
 * A for loop, which is a while loop with a step taken at the end of every
 * pass: the left child holds the condition, the middle child the body and
 * the right child the statement which runs at the end of the pass. The
 * right child is null when the loop was written without one. A for loop is
 * a node of its own and not a while loop because a `continue` goes on to
 * the step rather than back to the condition.
 */
class AstFor : public AstNode
{
public:
    AstFor(AstNodePtr& condition, AstNodePtr& body, AstNodePtr& post);
    ~AstFor() = default;
};

/*
 * Leave the innermost loop, skipping the rest of the body of that pass.
 */
class AstBreak : public AstNode
{
public:
    AstBreak();
    ~AstBreak() = default;
};

/*
 * Begin the next pass of the innermost loop, which for a for loop runs the
 * step written at the end of it.
 */
class AstContinue : public AstNode
{
public:
    AstContinue();
    ~AstContinue() = default;
};

/*
 * One arm of a switch: the left child holds the compound statement which
 * runs when the arm is taken, and `value` is what the switch expression has
 * to equal for that to happen. A default arm is taken when no other arm's
 * value matches, so its value is not used.
 */
class AstCase : public AstNode
{
public:
    AstCase(AstNodePtr& body, int32 value, bool is_default);
    ~AstCase() = default;

public:
    int32 GetValue() const;
    bool IsDefault() const;

private:
    int32 value_;
    bool is_default_;
};

/*
 * A switch statement. The left child is the expression whose value chooses
 * an arm, and the arms are held in the order they are written. An arm which
 * does not end in a break falls through into the arm after it.
 */
class AstSwitch : public AstNode
{
public:
    AstSwitch(AstNodePtr& expression, std::vector<AstNodePtr>& cases);
    ~AstSwitch() = default;

public:
    const std::vector<AstNodePtr>& GetCases() const;

    /* The arms of a switch which is being worked on rather than read. */
    std::vector<AstNodePtr>& GetCases();

private:
    std::vector<AstNodePtr> cases_;
};

/*
 * The declaration of a variable. Its symbol records whether the storage is
 * global or belongs to the current function's stack frame.
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
 * A function declaration. The left child holds the body, local_size is
 * the space required by its parameters and local variables, and parameters
 * holds one symbol per parameter in the order they are declared.
 */
class AstFunction : public AstNode
{
public:
    AstFunction(AstNodePtr& body,
                const Symbol& symbol,
                int32 local_size,
                std::vector<Symbol>& parameters);
    ~AstFunction() = default;

public:
    const Symbol& GetSymbol() const;
    int32 GetLocalSize() const;
    const std::vector<Symbol>& GetParameters() const;

private:
    Symbol symbol_;
    int32 local_size_;
    std::vector<Symbol> parameters_;
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

/*
 * Scale the value of the left child by a size, which is what turns an
 * integer into an offset when it is added to or subtracted from a pointer.
 * The type of this node is the pointer the offset belongs to.
 */
class AstScale : public AstNode
{
public:
    AstScale(AstNodePtr& expression, PrimitiveType type, int32 size);
    ~AstScale() = default;

public:
    int32 GetSize() const;

private:
    int32 size_;
};

/*
 * Return the value of the left child from the enclosing function.
 */
class AstReturn : public AstNode
{
public:
    explicit AstReturn(AstNodePtr& expression);
    ~AstReturn() = default;
};

/*
 * Call a function, holding one expression per argument in the order they
 * are written. The type of this node is the return type of the function.
 */
class AstFuncCall : public AstNode
{
public:
    AstFuncCall(std::vector<AstNodePtr>& arguments, const Symbol& symbol);
    ~AstFuncCall() = default;

public:
    const Symbol& GetSymbol() const;
    const std::vector<AstNodePtr>& GetArguments() const;

    /* The arguments of a call which is being worked on rather than read. */
    std::vector<AstNodePtr>& GetArguments();

private:
    Symbol symbol_;
    std::vector<AstNodePtr> arguments_;
};

/*
 * The address of a variable, produced by the '&' operator. It does not
 * stand for a value, so it has no child.
 */
class AstAddress : public AstNode
{
public:
    explicit AstAddress(const Symbol& symbol);
    ~AstAddress() = default;

public:
    const Symbol& GetSymbol() const;

private:
    Symbol symbol_;
};

/*
 * The value a pointer points at, produced by the '*' operator. The left
 * child holds the pointer, and the type of this node is what it points at.
 */
class AstDeref : public AstNode
{
public:
    AstDeref(AstNodePtr& pointer, PrimitiveType type);
    ~AstDeref() = default;
};

/* A member selected from a struct value with the `.` operator. */
class AstMember : public AstNode {
public:
    AstMember(AstNodePtr& base, const Symbol& member, bool through_pointer);
    ~AstMember() override = default;

    const Symbol& GetMember() const;
    bool IsThroughPointer() const;

private:
    Symbol member_;
    bool through_pointer_;
};

/* The address of an lvalue which is not a standalone variable. */
class AstAddressExpr : public AstNode {
public:
    explicit AstAddressExpr(AstNodePtr& lvalue);
    ~AstAddressExpr() override = default;
};

/*
 * A ternary expression, whose three parts are the condition, the value
 * taken when the condition holds and the value taken when it does not.
 * Only one of the two values is ever worked out, which is what sets this
 * apart from every other expression.
 */
class AstTernary : public AstNode
{
public:
    AstTernary(AstNodePtr& condition,
               AstNodePtr& when_true,
               AstNodePtr& when_false,
               PrimitiveType type,
               const std::shared_ptr<CompositeType>& composite);
    ~AstTernary() = default;
};

/*
 * The value of the left child, taken as another type. Nothing has to happen
 * to the value itself: the type says how it is read, stored and worked with
 * from this point on, and how much of it the new type keeps.
 */
class AstCast : public AstNode
{
public:
    AstCast(AstNodePtr& expression, const TypeRef& type);
    ~AstCast() = default;
};

/*
 * A unary operation on the value of the left child. The tag of the node
 * says which one: A_AstNegate flips the sign of an integer, A_AstInvert
 * flips every one of its bits, A_AstLogNot answers whether it is zero, and
 * A_AstToBool turns any value into the zero or one which the condition of
 * a selection or a loop statement tests.
 */
class AstUnary : public AstNode
{
public:
    AstUnary(AstNodePtr& expression, AstNodeTag tag, PrimitiveType type);
    ~AstUnary() = default;
};

/*
 * The increment or the decrement of a variable, written '++' or '--'. The
 * delta is what is added, and post records whether the operator was
 * written after the variable, in which case the value of the expression is
 * the one the variable held before.
 */
class AstIncDec : public AstNode
{
public:
    AstIncDec(const Symbol& symbol, int32 delta, bool post);
    ~AstIncDec() = default;

public:
    const Symbol& GetSymbol() const;
    int32 GetDelta() const;
    bool IsPost() const;

private:
    Symbol symbol_;
    int32 delta_;
    bool post_;
};

}   /* namespace nuocc */
