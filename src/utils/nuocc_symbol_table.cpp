#include "utils/nuocc_symbol_table.hpp"

namespace nuocc
{

std::optional<Symbol> SymbolTable::FindSymbol(const std::string& name) const
{
    /* The most recent definition of a name wins. */
    for (auto it = symbols_.rbegin(); it != symbols_.rend(); ++it)
    {
        if (it->name == name)
            return *it;
    }

    return std::nullopt;
}

void SymbolTable::AddSymbol(const Symbol& symbol)
{
    symbols_.push_back(symbol);
}

idx_t SymbolTable::Mark() const
{
    return symbols_.size();
}

void SymbolTable::Restore(idx_t mark)
{
    symbols_.resize(mark);
}

}   /* namespace nuocc */
