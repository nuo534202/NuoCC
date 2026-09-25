#include "utils/nuocc_type_check.hpp"

namespace nuocc
{

TypeMatch MatchTypes(PrimitiveType left,
    PrimitiveType right,
    bool only_widen_left)
{
    /* A void value cannot take part in an expression. */
    if (left == PrimitiveType::kVoid || right == PrimitiveType::kVoid)
        return TypeMatch{};

    /* The same type on both sides needs no conversion. */
    if (left == right)
        return TypeMatch{.compatible = true};

    /* A char always widens to an int. */
    if (left == PrimitiveType::kChar && right == PrimitiveType::kInt)
        return TypeMatch{.compatible = true, .widen_left = true};

    if (left == PrimitiveType::kInt && right == PrimitiveType::kChar)
    {
        if (only_widen_left)
            return TypeMatch{};

        return TypeMatch{.compatible = true, .widen_right = true};
    }

    /* Anything remaining is compatible for now. */
    return TypeMatch{.compatible = true};
}

}   /* namespace nuocc */
