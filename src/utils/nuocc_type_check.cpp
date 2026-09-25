#include "utils/nuocc_type_check.hpp"

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
        default:
            return 0;
    }
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
