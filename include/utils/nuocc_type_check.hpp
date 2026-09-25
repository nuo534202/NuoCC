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
 * The size in bytes of a primitive type on the target machine. A size of
 * zero means the type cannot hold a value at all.
 */
int32 PrimitiveSize(PrimitiveType type);

/*
 * The type which is a pointer to the given type, and the type a given
 * pointer type points at. Both die on a type which has no answer.
 */
PrimitiveType PointerTo(PrimitiveType type);
PrimitiveType ValueAt(PrimitiveType type);

/*
 * Match the primitive types of the two operands of an operator. Two types
 * are compatible when they are the same or when the narrower one can be
 * widened to the wider one.
 *
 * When only_widen_left is set the right operand keeps its type, so the
 * two are rejected when it is the right one which is narrower. That is
 * what stops a wide value from being stored into a narrow variable.
 */
TypeMatch MatchTypes(PrimitiveType left,
                     PrimitiveType right,
                     bool only_widen_left);

}   /* namespace nuocc */
