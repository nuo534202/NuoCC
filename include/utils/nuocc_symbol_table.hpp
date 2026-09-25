#pragma once

#include <optional>
#include <string>
#include <vector>

#include "utils/nuocc_types.hpp"

namespace nuocc
{

class SymbolTable
{
public:
    SymbolTable() = default;
    ~SymbolTable() = default;

public:
    /* Return the symbol with this name, or nothing when it is unknown. */
    std::optional<Symbol> FindSymbol(const std::string& name) const;
    void AddSymbol(const Symbol& symbol);

private:
    std::vector<Symbol> symbols_;
};

}   /* namespace nuocc */
