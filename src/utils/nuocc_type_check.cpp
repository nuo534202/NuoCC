#include "utils/nuocc_type_check.hpp"

#include <stdlib.h>

#include <iostream>

namespace nuocc
{

bool IsIntType(PrimitiveType type)
{
    switch (type)
    {
        case PrimitiveType::kChar:
        case PrimitiveType::kInt:
        case PrimitiveType::kLong:
            return true;
        default:
            return false;
    }
}

bool IsPointerType(PrimitiveType type)
{
    switch (type)
    {
        case PrimitiveType::kVoidPtr:
        case PrimitiveType::kCharPtr:
        case PrimitiveType::kIntPtr:
        case PrimitiveType::kLongPtr:
            return true;
        default:
            return false;
    }
}

int32 PrimitiveSize(PrimitiveType type)
{
    switch (type)
    {
        case PrimitiveType::kChar:
            return 1;
        case PrimitiveType::kInt:
            return 4;
        case PrimitiveType::kLong:
            return 8;
        /* Every pointer takes the same amount of room, whatever it points at. */
        case PrimitiveType::kVoidPtr:
        case PrimitiveType::kCharPtr:
        case PrimitiveType::kIntPtr:
        case PrimitiveType::kLongPtr:
            return 8;
        default:
            return 0;
    }
}

PrimitiveType PointerTo(PrimitiveType type)
{
    switch (type)
    {
        case PrimitiveType::kVoid:
            return PrimitiveType::kVoidPtr;
        case PrimitiveType::kChar:
            return PrimitiveType::kCharPtr;
        case PrimitiveType::kInt:
            return PrimitiveType::kIntPtr;
        case PrimitiveType::kLong:
            return PrimitiveType::kLongPtr;
        default:
            break;
    }

    std::cerr << "Error: there is no pointer to this type yet!" << std::endl;
    std::exit(1);
}

PrimitiveType ValueAt(PrimitiveType type)
{
    switch (type)
    {
        case PrimitiveType::kVoidPtr:
            return PrimitiveType::kVoid;
        case PrimitiveType::kCharPtr:
            return PrimitiveType::kChar;
        case PrimitiveType::kIntPtr:
            return PrimitiveType::kInt;
        case PrimitiveType::kLongPtr:
            return PrimitiveType::kLong;
        default:
            break;
    }

    std::cerr << "Error: this type is not a pointer!" << std::endl;
    std::exit(1);
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
     * A pointer fits a pointer of the same type as long as it does not
     * take part in an operation: pointer arithmetic is not arithmetic on
     * the addresses themselves, it is an offset scaled by the size of what
     * the pointer points at.
     */
    if (IsPointerType(type) && !op && type == wanted)
        return true;

    if (op == T_Plus || op == T_Minus)
    {
        if (IsIntType(type) && IsPointerType(wanted))
        {
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

}   /* namespace nuocc */
