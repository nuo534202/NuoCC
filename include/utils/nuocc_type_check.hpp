#pragma once

#include <optional>

#include "nodes/nuocc_ast_nodes.hpp"
#include "nodes/nuocc_nodes_tag.hpp"
#include "utils/nuocc_types.hpp"

namespace nuocc
{

/*
 * Whether a type holds an integer of some size, and whether it holds the
 * address of a value of some other type. The two groups are what the
 * conversions below are written in terms of.
 */
bool IsIntType(PrimitiveType type);
bool IsPointerType(PrimitiveType type);

/*
 * How many levels of indirection a type has, and the primitive type which
 * is left when they are taken away: a char ** has two levels, and char
 * beneath them.
 */
uint8 Indirection(PrimitiveType type);
PrimitiveType BaseType(PrimitiveType type);

/*
 * The size in bytes of a primitive type on the target machine. A size of
 * zero means the type cannot hold a value at all. A struct has no size of
 * its own, so ask ValueSize for one.
 */
int32 PrimitiveSize(PrimitiveType type);

/*
 * The room one value of a type takes, and what that value lines up to. A
 * struct takes the room its definition asks for, which is why the type
 * comes with the definition it is of.
 */
int32 ValueSize(PrimitiveType type,
                const std::shared_ptr<CompositeType>& composite);
int32 ValueAlignment(PrimitiveType type,
                     const std::shared_ptr<CompositeType>& composite);

/*
 * The number of bytes a symbol's storage takes: one value for a variable,
 * one value for every element of an array.
 */
int32 SymbolStorageSize(const Symbol& symbol);
int32 SymbolAlignment(const Symbol& symbol);

/*
 * The type which is a pointer to the given type, and the type a given
 * pointer type points at, which is one level of indirection less. Both die
 * on a type which has no answer.
 */
PrimitiveType PointerTo(PrimitiveType type);
PrimitiveType ValueAt(PrimitiveType type);

/*
 * Make the value of a tree fit the type wanted, and replace the tree with
 * the widened or the scaled one when that is what it takes. Returns false
 * when there is no way to make it fit, in which case the tree is left
 * alone.
 *
 * The operator is the one the tree takes part in, if any. It matters
 * because an integer may only be added to or subtracted from a pointer,
 * which is where it becomes an offset and has to be scaled. A tree which
 * takes part in no operation, such as the value of an assignment or of a
 * return, is given no operator.
 */
bool ModifyType(AstNodePtr& tree,
                PrimitiveType wanted,
                std::optional<NodeTag> op);

}   /* namespace nuocc */
