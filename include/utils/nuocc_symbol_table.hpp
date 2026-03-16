#pragma once

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
    /*
     *  return value: the index + 1 of the symbol, 0 if not found
     */
    idx_t FindSymbol(const Symbol& symbol);
    void AddSymbol(const Symbol& symbol);

private:
    std::vector<Symbol> symbols_;
};

}   /* namespace nuocc */