#pragma once

#include <stdexcept>

namespace nuocc
{

// A diagnosed compilation failure whose message is printed at the entry point.
class CompilerError : public std::runtime_error
{
public:
    using std::runtime_error::runtime_error;
};

}   // namespace nuocc
