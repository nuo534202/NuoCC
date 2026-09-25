#include "utils/nuocc_type_check.hpp"

#include <stdlib.h>

#include <iostream>

namespace nuocc
{

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

TypeMatch MatchTypes(PrimitiveType left,
    PrimitiveType right,
    bool only_widen_left)
{
    /* The same type on both sides needs no conversion. */
    if (left == right)
        return TypeMatch{.compatible = true};

    int32 left_size = PrimitiveSize(left);
    int32 right_size = PrimitiveSize(right);

    /* A type with no size, void or none, cannot hold a value. */
    if (left_size == 0 || right_size == 0)
        return TypeMatch{};

    if (left_size < right_size)
        return TypeMatch{.compatible = true, .widen_left = true};

    if (right_size < left_size)
    {
        if (only_widen_left)
            return TypeMatch{};

        return TypeMatch{.compatible = true, .widen_right = true};
    }

    /* Two different types of the same size are compatible as they are. */
    return TypeMatch{.compatible = true};
}

}   /* namespace nuocc */
