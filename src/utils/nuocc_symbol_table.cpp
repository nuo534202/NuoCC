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

bool SymbolTable::UpdateSymbol(const Symbol& symbol)
{
    for (auto it = symbols_.rbegin(); it != symbols_.rend(); ++it)
    {
        if (it->name == symbol.name)
        {
            *it = symbol;
            return true;
        }
    }

    return false;
}

void SymbolTable::AddTag(const std::shared_ptr<CompositeType>& type)
{
    tags_.push_back(type);
}

std::shared_ptr<CompositeType> SymbolTable::FindTag(
    const std::string& name) const
{
    /* The most recent definition of a tag wins, as it does for a name. */
    for (auto it = tags_.rbegin(); it != tags_.rend(); ++it)
    {
        if ((*it)->name == name)
            return *it;
    }

    return nullptr;
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
