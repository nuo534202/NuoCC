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
        tree = FoldedLeaf(folded, tree->GetType());
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
