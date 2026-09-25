#pragma once

#include <stdint.h>

#include <memory>
#include <string>

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
 * The primitive type of a value. kNone marks the trees which are not
 * expressions at all, such as statements and declarations.
 */
enum class PrimitiveType : uint8
{
    kNone = 0,
    kVoid,
    kChar,
    kInt
};

/* What a symbol names. */
enum class StructuralType : uint8
{
    kVariable = 0,
    kFunction
};

/* An entry of the symbol table. */
struct Symbol
{
    std::string name;
    PrimitiveType type = PrimitiveType::kNone;
    StructuralType stype = StructuralType::kVariable;
};

using NodePtr = std::unique_ptr<class Node>;
using AstNodePtr = std::unique_ptr<class AstNode>;

}   /* namespace nuocc */
