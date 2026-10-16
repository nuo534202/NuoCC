#pragma once

#include <stdint.h>

#include <memory>
#include <string>
#include <variant>
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
 * How much room each of the language's types takes on the machine the
 * compiler is built for. What a value takes is what its type says, so the
 * width of an int is the width of the machine's int rather than a number
 * of the compiler's own: an int is four bytes on the machines this
 * compiler began with and two on the 6809, and the room a frame, a struct
 * or a sizeof takes is worked out from these.
 */
struct TypeSizes
{
    int32 char_size;
    int32 short_size;
    int32 int_size;
    int32 long_size;
    int32 pointer_size;
};

/*
 * The widths of the types on the machine the target of this build writes
 * code for. Each target names its own, and the build picks one of them.
 */
const TypeSizes& TargetSizes();

/*
 * The type of a value. The bottom four bits of a type are how many levels
 * of indirection it has and the rest of it is which primitive type it is
 * built from, so a char is kChar and a char ** is two levels above it.
 * Only the primitive types are named here: the helpers in
 * utils/nuocc_type_check.hpp are what give a type one more level of
 * indirection and take one away, and nothing else reads the levels for
 * itself. kNone marks the trees which are not expressions at all, such as
 * statements and declarations.
 *
 * A char has no sign, since it holds the characters of a machine; a short,
 * an int and a long have one, so a value of one of those is read back as a
 * number which may be below nothing.
 */
enum class PrimitiveType : uint8
{
    kNone = 0x00,
    kVoid = 0x10,
    kChar = 0x20,
    kShort = 0x30,
    kInt = 0x40,
    kLong = 0x50,
    /*
     * A struct and a union are composite types. Which one a value is of
     * is said by the definition it carries and not by the type value, so
     * that either one and a pointer to it are put together the same way a
     * char and its pointers are.
     */
    kComposite = 0x60
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
    kArray,
    /*
     * The name an `enum` gives to a number. It is a name for a value and
     * not a place which holds one, so it is never a variable: reading it
     * stands for the number it was declared with, and nothing may be
     * stored into it.
     */
    kEnumValue,
    /*
     * Another name for a type, made with `typedef`. A typedef stands
     * wherever the type it names may stand, and the type checks see the
     * type it is a name for rather than the name itself.
     */
    kTypedef
};

enum class StorageClass : uint8
{
    kGlobal = 0,
    kLocal,
    /* A member of a struct lives inside a value of the struct rather than
     * in storage of its own. */
    kMember,
    /*
     * A name declared with `extern`: it is visible like a global, but its
     * storage is given in another source file, so none is emitted for it
     * here.
     */
    kExtern
};

/*
 * One value a global variable is given where it is declared. A variable of
 * type char * is given the characters of a string, whose address the value
 * becomes; every other variable is given a number, so a value is one of the
 * two and the type of the variable says which.
 */
using InitialValue = std::variant<int32, std::string>;

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
    /*
     * Whether the name was declared static, which means it is visible only
     * in the source file where it is declared. Its storage is put where it
     * stands like any other global one; what changes is that the assembler
     * is not told another file may reach it.
     */
    bool is_static = false;
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
    /*
     * Whether a function is declared to take more arguments than it names
     * parameters, which is what the '...' at the end of its parameter list
     * says. It is false for everything which is not a function, and for a
     * function which names every argument it takes.
     */
    bool is_variadic = false;
    /* The number of elements of an array, and zero for anything else. */
    int32 element_count = 0;
    /* Where a member starts, measured from the base of its struct. */
    int32 member_offset = 0;
    /* The number an enum value stands for, and zero for anything else. */
    int32 enum_value = 0;
    /*
     * The values a global variable is given where it is declared, in the
     * order they are written: one for the variable itself, or one for every
     * element of an array. It is empty when the variable was declared
     * without a value, in which case its storage starts out zero.
     */
    std::vector<InitialValue> initial_values = {};
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
 * The definition a tag names: a struct, a union or an enum. A struct and a
 * union carry the members they are made of in the order they were written
 * and the room a value of the type takes. A member is a symbol because a
 * member is a variable in everything but where it lives, which is what lets
 * one of them be an array, or a composite of its own.
 *
 * An enum is the third kind of tag, and the only one with no layout of its
 * own: it names no members and takes no room because a value of it is
 * simply an int. It keeps a place in the tag namespace so that `enum fred`
 * names the same tag wherever it is written, and so that the tag cannot be
 * reused as a struct or a union.
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
    /*
     * Whether this is an enum tag. An enum has no members and no size, so
     * the fields above say nothing about it; what sets it apart from a
     * struct or a union is only that `enum fred` is a name for an int.
     */
    bool is_enum = false;
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
