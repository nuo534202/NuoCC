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

    /*
     * Replace the latest declaration of a name, which is how a prototype
     * becomes the place a later definition of the function is kept. It
     * answers whether the name was known.
     */
    bool UpdateSymbol(const Symbol& symbol);
    idx_t Mark() const;
    void Restore(idx_t mark);

private:
    std::vector<Symbol> symbols_;
};

}   /* namespace nuocc */
