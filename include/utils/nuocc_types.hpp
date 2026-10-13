#pragma once

#include <stdint.h>

#include <memory>
#include <string>
#include <vector>

namespace nuocc
{

using int8 = int8_t;
using int16 = int16_t;
using int32 = int32_t;
using int64 = int64_t;
using uint8 = uint8_t;
using uint16 = uint16_t;
using uint32 = uint32_t;
using uint64 = uint64_t;

using idx_t = size_t;
/* register index */
using reg_idx = size_t;
/* assembly label number */
using label_idx = uint32;

/*
 * The registers the ABI gives to arguments set how many a call may pass
 * without reaching into the stack, and six is the number both targets
 * have in common.
 */
constexpr int32 kMaxArguments = 6;

/*
 * The type of a value. The bottom four bits of a type are how many levels
 * of indirection it has and the rest of it is which primitive type it is
 * built from, so a char is kChar and a char ** is two levels above it.
 * Only the primitive types are named here: the helpers in
 * utils/nuocc_type_check.hpp are what give a type one more level of
 * indirection and take one away, and nothing else reads the levels for
 * itself. kNone marks the trees which are not expressions at all, such as
 * statements and declarations.
 */
enum class PrimitiveType : uint8
{
    kNone = 0x00,
    kVoid = 0x10,
    kChar = 0x20,
    kInt = 0x30,
    kLong = 0x40,
    /*
     * A struct and a union are composite types. Which one a value is of
     * is said by the definition it carries and not by the type value, so
     * that either one and a pointer to it are put together the same way a
     * char and its pointers are.
     */
    kComposite = 0x50
};

/* The definition of a struct or a union, which a value of the type
 * carries so that its members can be reached from it. */
struct CompositeType;

/* What a symbol names. */
enum class StructuralType : uint8
{
    kVariable = 0,
    kFunction,
    /*
     * An array. Its `type` is the type of one element rather than of the
     * array itself, so that every element is found with the type checks
     * an expression is written with; `element_count` is how many of them
     * the declaration asked for.
     */
    kArray
};

enum class StorageClass : uint8
{
    kGlobal = 0,
    kLocal,
    /* A member of a struct lives inside a value of the struct rather than
     * in storage of its own. */
    kMember
};

/*
 * A type as it is written down: which primitive type, and the struct which
 * the value is of when it is one. The type value on its own cannot say
 * which struct a value is of, so the two travel together.
 */
struct TypeRef
{
    PrimitiveType type = PrimitiveType::kNone;
    std::shared_ptr<CompositeType> composite = nullptr;

    bool operator==(const TypeRef&) const = default;
};

/* An entry of the symbol table. */
struct Symbol
{
    std::string name;
    PrimitiveType type = PrimitiveType::kNone;
    /*
     * The struct a value of this type is. A pointer to a struct carries
     * the definition it points at, so the members of the type a value is
     * of are reachable however many levels of pointer there are.
     */
    std::shared_ptr<CompositeType> composite = nullptr;
    StructuralType stype = StructuralType::kVariable;
    StorageClass storage = StorageClass::kGlobal;
    int32 stack_offset = 0;
    /*
     * The types of the parameters of a function, in the order they are
     * declared. It is empty for everything which is not a function.
     */
    std::vector<TypeRef> parameter_types = {};
    /*
     * Whether a function has been given a body. A prototype has none yet,
     * and the flag is false for everything which is not a function.
     */
    bool has_body = false;
    /* The number of elements of an array, and zero for anything else. */
    int32 element_count = 0;
    /* Where a member starts, measured from the base of its struct. */
    int32 member_offset = 0;
};

/*
 * The two shapes a composite type can have. A struct lays its members out
 * one after another; a union lays every one of them at the same place.
 */
enum class CompositeKind : uint8
{
    kStruct = 0,
    kUnion
};

/*
 * The definition of a composite type: the tag it was declared with, the
 * members it is made of in the order they were written, and the room a
 * value of the type takes. A member is a symbol because a member is a
 * variable in everything but where it lives, which is what lets one of
 * them be an array, or a composite of its own.
 *
 * A struct may contain a pointer to itself, so the definition is known by
 * its tag before its members have been read. Until they have, its size is
 * zero, which is a size no value can have.
 */
struct CompositeType
{
    CompositeKind kind = CompositeKind::kStruct;
    /* The tag it was declared with, empty when it was written without one. */
    std::string name;
    std::vector<Symbol> members;
    /* How many bytes a value of the type takes, and what it lines up to. */
    int32 size = 0;
    int32 alignment = 1;
};

using NodePtr = std::unique_ptr<class Node>;
using AstNodePtr = std::unique_ptr<class AstNode>;

/*
 * The whole program. What a declaration at the top level declares is kept
 * apart: the storage for a global variable is emitted once before any
 * code, while each function is emitted where it stands.
 */
struct Program
{
    /* Every composite type the program declared, in declaration order. */
    std::vector<std::shared_ptr<CompositeType>> types;
    /* Every function of the program, in the order it was declared. */
    std::vector<AstNodePtr> functions;
    /* Every global variable, in the order it was declared. */
    std::vector<AstNodePtr> globals;
};

}   /* namespace nuocc */
