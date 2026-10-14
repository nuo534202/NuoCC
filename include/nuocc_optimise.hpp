#pragma once

#include "nodes/nuocc_ast_nodes.hpp"
#include "utils/nuocc_types.hpp"

namespace nuocc
{

/*
 * Optimise a tree of statements and expressions, and answer with the tree
 * which stands in its place.
 *
 * The only optimisation so far is constant folding: an operation whose
 * operands the compiler already knows is worked out while the program is
 * being compiled, and the smallest tree which gives the same value is left
 * in its place. The value of every operation is worked out in an integer
 * wider than the language has, and is only folded when it fits an int, so
 * that a folded tree always gives the value the operation would have given
 * when the program ran.
 */
AstNodePtr Optimise(AstNodePtr tree);

}   /* namespace nuocc */
