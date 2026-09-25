#pragma once

#include "utils/nuocc_types.hpp"

namespace nuocc
{

/* The outcome of matching the primitive types of two operands. */
struct TypeMatch
{
    /* The two types may appear together in one expression. */
    bool compatible = false;
    /* The left operand is the narrower one and has to be widened. */
    bool widen_left = false;
    /* The right operand is the narrower one and has to be widened. */
    bool widen_right = false;
};

/*
 * Match the primitive types of the two operands of an operator.
 *
 * When only_widen_left is set the right operand keeps its type, so the
 * two types are rejected when it is the left one which is wider.
 * Assignments use that to stop a wide value from being stored into a
 * narrow variable.
 */
TypeMatch MatchTypes(PrimitiveType left,
                     PrimitiveType right,
                     bool only_widen_left);

}   /* namespace nuocc */
