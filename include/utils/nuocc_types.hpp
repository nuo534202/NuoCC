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

using Symbol = std::string;

using NodePtr = std::unique_ptr<class Node>;
using AstNodePtr = std::unique_ptr<class AstNode>;

}   /* namespace nuocc */