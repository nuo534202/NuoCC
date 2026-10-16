#include "utils/nuocc_type_check.hpp"

#include <stdlib.h>

#include <algorithm>
#include <iostream>

namespace nuocc
{

namespace
{

/*
 * The bits of a type which count its levels of indirection. The rest of
 * the type says which primitive type it is built from, so no level of
 * indirection can reach into it.
 */
constexpr uint8 kIndirectionMask = 0x0F;

/* A type with the given number of levels of indirection. */
PrimitiveType WithIndirection(PrimitiveType type, uint8 levels)
{
    const uint8 base = static_cast<uint8>(type) & ~kIndirectionMask;

    return static_cast<PrimitiveType>(base | levels);
}

}   /* namespace */

bool ValueFitsType(int64 value, PrimitiveType type)
{
    /*
     * A char has no sign, so it holds 0 to 255 whatever the machine it is
     * on. Everything else has a sign, and holds as far below nothing as
     * the room its type takes allows: sixteen bits reach -32768 to 32767.
     */
    if (type == PrimitiveType::kChar)
        return value >= 0 && value < 256;

    const int32 size = PrimitiveSize(type);

    /* A type which takes no room holds nothing at all. */
    if (size == 0)
        return false;

    /* A value of eight bytes holds every number the compiler has. */
    if (size >= 8)
        return true;

    const int64 limit = static_cast<int64>(1) << (size * 8 - 1);

    return value >= -limit && value < limit;
}

bool IsIntType(PrimitiveType type)
{
    if (IsPointerType(type))
        return false;

    switch (type)
    {
        case PrimitiveType::kChar:
        case PrimitiveType::kShort:
        case PrimitiveType::kInt:
        case PrimitiveType::kLong:
            return true;
        default:
            return false;
    }
}

bool IsPointerType(PrimitiveType type)
{
    return Indirection(type) != 0;
}

uint8 Indirection(PrimitiveType type)
{
    return static_cast<uint8>(type) & kIndirectionMask;
}

PrimitiveType BaseType(PrimitiveType type)
{
    return WithIndirection(type, 0);
}

int32 PrimitiveSize(PrimitiveType type)
{
    const TypeSizes& sizes = TargetSizes();

    /* Every pointer takes the same amount of room, whatever it points at. */
    if (IsPointerType(type))
        return sizes.pointer_size;

    switch (type)
    {
        case PrimitiveType::kChar:
            return sizes.char_size;
        case PrimitiveType::kShort:
            return sizes.short_size;
        case PrimitiveType::kInt:
            return sizes.int_size;
        case PrimitiveType::kLong:
            return sizes.long_size;
        default:
            return 0;
    }
}

int32 ValueSize(PrimitiveType type,
    const std::shared_ptr<CompositeType>& composite)
{
    /* A pointer to a struct is a pointer like any other. It is a struct
     * value whose room comes from the definition and not from the type. */
    if (type == PrimitiveType::kComposite)
        return composite ? composite->size : 0;

    return PrimitiveSize(type);
}

int32 ValueAlignment(PrimitiveType type,
    const std::shared_ptr<CompositeType>& composite)
{
    if (type == PrimitiveType::kComposite)
        return composite ? composite->alignment : 1;

    const int32 size = PrimitiveSize(type);

    /*
     * A value stands where a whole number of its own width along begins,
     * which is what makes a value of four bytes or more stand at the
     * widest alignment a machine of this compiler has. A type which takes
     * no room at all is nothing to line up against.
     */
    return size == 0 ? 1 : std::min<int32>(size, 8);
}

int32 SymbolStorageSize(const Symbol& symbol)
{
    const int32 element = ValueSize(symbol.type, symbol.composite);

    if (symbol.stype != StructuralType::kArray)
        return element;

    return element * symbol.element_count;
}

int32 SymbolAlignment(const Symbol& symbol)
{
    return ValueAlignment(symbol.type, symbol.composite);
}

PrimitiveType PointerTo(PrimitiveType type)
{
    if (BaseType(type) == PrimitiveType::kNone)
    {
        std::cerr << "Error: there is no pointer to this type!" << std::endl;
        std::exit(1);
    }

    if (Indirection(type) == kIndirectionMask)
    {
        std::cerr << "Error: a type cannot be a pointer that many times!";
        std::cerr << std::endl;
        std::exit(1);
    }

    return WithIndirection(type, Indirection(type) + 1);
}

PrimitiveType ValueAt(PrimitiveType type)
{
    if (!IsPointerType(type))
    {
        std::cerr << "Error: this type is not a pointer!" << std::endl;
        std::exit(1);
    }

    return WithIndirection(type, Indirection(type) - 1);
}

namespace
{

/*
 * Two integer types: the narrower one is widened to the wider one, and one
 * which is already wider than the type wanted cannot be narrowed to fit.
 */
bool ModifyIntType(AstNodePtr& tree, PrimitiveType type, PrimitiveType wanted)
{
    if (type == wanted)
        return true;

    if (PrimitiveSize(type) > PrimitiveSize(wanted))
        return false;

    tree = std::make_unique<AstWiden>(tree, wanted);

    return true;
}

}   /* namespace */

bool ModifyType(AstNodePtr& tree,
    PrimitiveType wanted,
    std::optional<NodeTag> op)
{
    PrimitiveType type = tree->GetType();

    if (IsIntType(type) && IsIntType(wanted))
        return ModifyIntType(tree, type, wanted);

    /*
     * Two addresses may be compared with each other whatever they point at,
     * since the question is which address is the lower one and that does not
     * depend on what stands at either of them.
     */
    if (IsPointerType(type) && IsPointerType(wanted) && op &&
        IsComparisonOp(*op))
        return true;

    /*
     * A pointer fits another pointer as long as it takes part in no other
     * operation, since pointer arithmetic is not arithmetic on the addresses
     * themselves but an offset scaled by the size of what the pointer points
     * at. A pointer to void is the one pointer which stands for any other,
     * so it may be stored where any pointer type is wanted and any pointer
     * may be stored where one is.
     */
    if (IsPointerType(type) && IsPointerType(wanted) && !op &&
        (type == wanted ||
         ValueAt(type) == PrimitiveType::kVoid ||
         ValueAt(wanted) == PrimitiveType::kVoid))
        return true;

    if (op == T_Plus || op == T_Minus)
    {
        if (IsIntType(type) && IsPointerType(wanted))
        {
            /*
             * How far along the next value is is the size of what the
             * pointer points at, and a composite type only knows that
             * once its definition says so and not from the type value,
             * which the tree being scaled here does not carry. Stepping a
             * pointer to one along is therefore refused rather than scaled
             * by a size of zero.
             */
            if (ValueAt(wanted) == PrimitiveType::kComposite)
            {
                std::cerr << "syntax error: arithmetic on a pointer to a";
                std::cerr << " composite value is not supported yet!" << std::endl;
                std::exit(1);
            }

            /*
             * The integer becomes an offset into what the pointer points
             * at, so it is scaled by the size of one of those. A size of
             * one scales by one, which leaves the value alone.
             */
            tree = std::make_unique<AstScale>(tree,
                                              wanted,
                                              PrimitiveSize(ValueAt(wanted)));
            return true;
        }
    }

    return false;
}

bool ModifyType(AstNodePtr& tree, const TypeRef& wanted,
                std::optional<NodeTag> op) {
    if (BaseType(tree->GetType()) == PrimitiveType::kComposite &&
        BaseType(wanted.type) == PrimitiveType::kComposite &&
        tree->GetComposite() != wanted.composite) {
        return false;
    }
    return ModifyType(tree, wanted.type, op);
}

}   /* namespace nuocc */
