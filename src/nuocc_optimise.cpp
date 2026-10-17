#include "nuocc_optimise.hpp"

#include <limits>

#include "utils/nuocc_type_check.hpp"

namespace nuocc
{

namespace
{

/* Whether a node holds a number the compiler already knows. */
bool IsLiteral(const AstNodePtr& node)
{
    return node && node->GetAstNodeTag() == A_AstIntLit;
}

int32 LiteralValue(const AstNodePtr& node)
{
    return static_cast<const AstIntLit&>(*node).GetValue();
}

/*
 * Work out a binary operation on two known numbers and answer whether it is
 * one the compiler can work out at all, leaving the answer in `value`. The
 * operation is done in a wider integer than the language has, so that a
 * result which does not fit can be recognised rather than wrapping round.
 */
bool FoldBinary(NodeTag op_type, int64 left, int64 right, int64& value)
{
    switch (op_type)
    {
        case T_Plus:
            value = left + right;
            return true;
        case T_Minus:
            value = left - right;
            return true;
        case T_Star:
            value = left * right;
            return true;
        case T_Slash:
            /*
             * Dividing by zero has to fail while the program runs, the way
             * it does when the compiler leaves the operation alone, so it
             * is not worked out here.
             */
            if (right == 0)
                return false;

            value = left / right;
            return true;
        case T_Percent:
            /* Much as a division by nothing, a remainder of nothing has to
             * be left for the program to fail over. */
            if (right == 0)
                return false;

            /*
             * The one remainder which does not fit the type it is worked out
             * at is left alone as well, since the machine answers that one
             * in a way of its own.
             */
            if (left == std::numeric_limits<int32>::min() && right == -1)
                return false;

            value = left % right;
            return true;
        case T_Amper:
            value = left & right;
            return true;
        case T_Or:
            value = left | right;
            return true;
        case T_Xor:
            value = left ^ right;
            return true;
        /*
         * A shift is done on a whole register and the machine takes only
         * the low bits of the count, so a count of a whole register or
         * more is left for the program to work out.
         */
        case T_LShift:
            if (right < 0 || right >= 64)
                return false;

            value = static_cast<int64>(static_cast<uint64>(left)
                                       << static_cast<uint64>(right));
            return true;
        case T_RShift:
            if (right < 0 || right >= 64)
                return false;

            /* A shift of a negative value fills the top bits in with
             * zeroes, so the register is shifted as an unsigned one. */
            value = static_cast<int64>(static_cast<uint64>(left)
                                       >> static_cast<uint64>(right));
            return true;
        case T_EQ:
            value = left == right ? 1 : 0;
            return true;
        case T_NE:
            value = left != right ? 1 : 0;
            return true;
        case T_LT:
            value = left < right ? 1 : 0;
            return true;
        case T_GT:
            value = left > right ? 1 : 0;
            return true;
        case T_LE:
            value = left <= right ? 1 : 0;
            return true;
        case T_GE:
            value = left >= right ? 1 : 0;
            return true;
        /*
         * Both sides of a logical operation have already been turned into
         * the zero or the one which says whether they hold, so what is left
         * of the operation is the question of whether both, or either, do.
         */
        case T_LogAnd:
            value = (left != 0 && right != 0) ? 1 : 0;
            return true;
        case T_LogOr:
            value = (left != 0 || right != 0) ? 1 : 0;
            return true;
        default:
            return false;
    }
}

/*
 * Work out a unary operation on a known number. A widening leaves the
 * number as it is and only says that it is a wider one, which is what lets
 * `3000 + 1` be worked out: the literal one is widened before it is added
 * to the 3000.
 */
bool FoldUnary(AstNodeTag tag, int64 operand, int64& value)
{
    switch (tag)
    {
        case A_AstWiden:
            value = operand;
            return true;
        case A_AstNegate:
            value = -operand;
            return true;
        case A_AstInvert:
            value = ~operand;
            return true;
        case A_AstLogNot:
            value = operand == 0 ? 1 : 0;
            return true;
        case A_AstToBool:
            value = operand != 0 ? 1 : 0;
            return true;
        default:
            return false;
    }
}

/*
 * The number the tree became, in place of the tree itself. Its type is the
 * type of the operation which was worked out, so that everything reading it
 * afterwards reads the type the operation was done at.
 */
AstNodePtr FoldedLeaf(int64 value, PrimitiveType type)
{
    AstNodePtr left = nullptr;
    AstNodePtr right = nullptr;

    return std::make_unique<AstIntLit>(left, right,
                                       static_cast<int32>(value), type);
}

/*
 * Whether an operation answers with the value of the side which is not the
 * number given, which is what lets the operation be taken away and that
 * side left in its place.
 *
 * Which number does nothing depends on the operation, and so does the side
 * it does nothing on: `x + 0`, `0 + x`, `x - 0`, `x * 1`, `1 * x`, `x / 1`,
 * `x | 0`, `0 | x`, `x ^ 0`, `0 ^ x`, `x << 0` and `x >> 0` are all the
 * value of the other side, while `0 - x` and `0 / x` are not, which is why
 * the side is asked about as well as the number.
 *
 * An operation which answers with the number itself, such as `x & 0` or
 * `x * 0`, is not one of these. Taking such an operation away would take
 * the other side away with it, and the other side may be something which
 * has to be worked out even when the answer does not depend on it: a call
 * which prints is not made to go away by writing `0` beside it.
 */
bool LeavesOtherSideAlone(NodeTag op_type, bool right_side, int32 value)
{
    /*
     * An addition and a bitwise or or exclusive or do nothing with a zero
     * wherever the zero stands, and a multiplication does nothing with a
     * one.
     */
    if (op_type == T_Plus || op_type == T_Or || op_type == T_Xor)
        return value == 0;

    if (op_type == T_Star)
        return value == 1;

    /*
     * What the other side is taken from, divided by or moved by stands on
     * the right, so only a number there leaves the other side alone.
     */
    if (!right_side)
        return false;

    if (op_type == T_Minus || op_type == T_LShift || op_type == T_RShift)
        return value == 0;

    return op_type == T_Slash && value == 1;
}

/*
 * Take away an operation which does nothing with one of its sides,
 * leaving the other side in its place, and answer whether there was one.
 *
 * What is left is already of the type the operation answers with, since
 * the two sides of an operation are made to agree on a type before it is
 * built, so nothing has to be widened or cut down where it now stands.
 */
bool TakeAwayIdentity(AstNodePtr& tree)
{
    const NodeTag op_type = static_cast<const AstOperator&>(*tree).GetOpType();

    if (IsLiteral(tree->GetLeft()) &&
        LeavesOtherSideAlone(op_type, false, LiteralValue(tree->GetLeft())))
    {
        tree = tree->ReleaseRight();
        return true;
    }

    if (IsLiteral(tree->GetRight()) &&
        LeavesOtherSideAlone(op_type, true, LiteralValue(tree->GetRight())))
    {
        tree = tree->ReleaseLeft();
        return true;
    }

    return false;
}

/*
 * How many places a power of two moves a value up by, and -1 for a number
 * which is not a power of two above one: a one moves a value up no places,
 * a two moves it up one, a four two, and so on.
 */
int32 PowerOfTwoPlaces(int32 value)
{
    if (value < 2)
        return -1;

    int32 places = 0;

    while ((value & 1) == 0)
    {
        value = value >> 1;
        places++;
    }

    return value == 1 ? places : -1;
}

/*
 * Write a multiplication by a power of two as a shift of the other side,
 * and answer whether there was one.
 *
 * A product by a power of two is the other side moved up as many places as
 * the power says, since every place up doubles the value, and a shift
 * moves a value up by a number of places in one step where a multiplication
 * works out a whole product: `x * 8` is `x << 3`. The two are the same
 * value whatever the other side is: moving a value up one place leaves
 * every bit of it standing one place higher with a zero at the bottom,
 * which is what doubling it does.
 *
 * A division by a power of two is not written this way. A division rounds
 * towards nothing, while a shift of a value below nothing moves the top
 * bits down with it, so the two answer with the same value only where the
 * value is not below nothing, which the compiler does not know.
 */
bool WriteMultiplyAsShift(AstNodePtr& tree)
{
    const AstOperator& op = static_cast<const AstOperator&>(*tree);

    if (op.GetOpType() != T_Star)
        return false;

    AstNodePtr value;
    int32 places = -1;

    if (IsLiteral(tree->GetLeft()))
        places = PowerOfTwoPlaces(LiteralValue(tree->GetLeft()));

    if (places > 0)
        value = tree->ReleaseRight();
    else
    {
        if (!IsLiteral(tree->GetRight()))
            return false;

        places = PowerOfTwoPlaces(LiteralValue(tree->GetRight()));

        if (places <= 0)
            return false;

        value = tree->ReleaseLeft();
    }

    const PrimitiveType type = tree->GetType();
    AstNodePtr places_leaf = FoldedLeaf(places, type);

    tree = std::make_unique<AstOperator>(value, places_leaf, T_LShift, type);
    return true;
}

/*
 * Write what is left of an operation another way where that can be done:
 * one which does nothing with one of its sides is taken away, and one which
 * multiplies by a power of two is written as a shift.
 */
void Simplify(AstNodePtr& tree)
{
    if (TakeAwayIdentity(tree))
        return;

    WriteMultiplyAsShift(tree);
}

void Fold(AstNodePtr& tree);

/*
 * Fold the trees below this node first, and then this node when it is an
 * operation on numbers the compiler knows. Working from the inside out is
 * what turns `2 * 3 + 4` into `6 + 4` and then into `10`.
 */
void FoldNode(AstNodePtr& tree)
{
    Fold(tree->GetLeft());
    Fold(tree->GetMid());
    Fold(tree->GetRight());

    const AstNodeTag tag = tree->GetAstNodeTag();
    int64 folded = 0;

    if (tag == A_AstOperator && IsLiteral(tree->GetLeft()) &&
        IsLiteral(tree->GetRight()))
    {
        const AstOperator& op = static_cast<const AstOperator&>(*tree);

        if (FoldBinary(op.GetOpType(),
                       LiteralValue(tree->GetLeft()),
                       LiteralValue(tree->GetRight()),
                       folded) &&
            ValueFitsType(folded, tree->GetType()))
            tree = FoldedLeaf(folded, tree->GetType());

        return;
    }

    /* Every other operation the compiler can work out has one operand.
     * FoldUnary answers for the tags which are not such an operation. */
    if (IsLiteral(tree->GetLeft()) &&
        FoldUnary(tag, LiteralValue(tree->GetLeft()), folded) &&
        ValueFitsType(folded, tree->GetType()))
    {
        tree = FoldedLeaf(folded, tree->GetType());
        return;
    }

    /*
     * What is left is an operation with a side the compiler does not know
     * the value of, which may still be an operation it can write another
     * way or one it does nothing with.
     */
    if (tag == A_AstOperator)
        Simplify(tree);
}

/*
 * Fold one tree, along with the arms of a switch and the arguments of a
 * call, which are held in lists of their own rather than as children.
 */
void Fold(AstNodePtr& tree)
{
    if (!tree)
        return;

    switch (tree->GetAstNodeTag())
    {
        case A_AstSwitch:
            Fold(tree->GetLeft());

            for (AstNodePtr& arm : static_cast<AstSwitch&>(*tree).GetCases())
                Fold(arm);
            return;

        case A_AstFuncCall:
            for (AstNodePtr& argument :
                 static_cast<AstFuncCall&>(*tree).GetArguments())
                Fold(argument);
            return;

        default:
            FoldNode(tree);
            return;
    }
}

}   /* namespace */

AstNodePtr Optimise(AstNodePtr tree)
{
    Fold(tree);

    return tree;
}

}   /* namespace nuocc */
