#pragma once

#include "nodes/nuocc_ast_nodes.hpp"
#include "utils/nuocc_types.hpp"

namespace nuocc
{

/*
 * Optimise a tree of statements and expressions, and answer with the tree
 * which stands in its place.
 *
 * Three things are done with the tree, each of which leaves a tree which
 * gives the same value as the one it was given:
 *
 *  - constant folding: an operation whose two sides the compiler already
 *    knows is worked out while the program is being compiled, and the
 *    number it comes to is left in its place. The value is worked out in
 *    an integer wider than the language has, and is only folded when it
 *    fits the type the operation answers with, so that a folded tree
 *    always gives the value the operation would have given when the
 *    program ran;
 *
 *  - taking away an operation which does nothing with one of its sides,
 *    such as `x + 0` or `x * 1`, which leaves the other side in its place.
 *    An operation which answers with the number instead of the other side,
 *    such as `x & 0`, is left alone, since taking it away would take the
 *    other side with it and that side may be something which has to be
 *    worked out;
 *
 *  - writing a multiplication by a power of two as a shift of the other
 *    side, such as `x * 8` as `x << 3`, which the machine works out in one
 *    step rather than by working out a whole product.
 */
AstNodePtr Optimise(AstNodePtr tree);

}   /* namespace nuocc */
