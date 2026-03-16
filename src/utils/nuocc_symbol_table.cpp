#include "utils/nuocc_symbol_table.hpp"

namespace nuocc
{

idx_t SymbolTable::FindSymbol(const Symbol& symbol)
{
    for (idx_t i = symbols_.size(); i; i--)
    {
        if (symbols_[i - 1] == symbol)
            return i;
    }

    return 0;
}

void SymbolTable::AddSymbol(const Symbol& symbol)
{
    symbols_.push_back(symbol);
}

}   /* namespace nuocc */