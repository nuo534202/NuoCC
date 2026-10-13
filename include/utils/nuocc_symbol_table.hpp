#pragma once

#include <memory>
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

    /*
     * The tag of a struct is looked up on its own, because C lets a tag
     * and a variable share a name: `struct fred` and `int fred` may both
     * be declared, and only a tag is what `struct fred` names.
     */
    void AddTag(const std::shared_ptr<CompositeType>& type);
    std::shared_ptr<CompositeType> FindTag(const std::string& name) const;

private:
    std::vector<Symbol> symbols_;
    std::vector<std::shared_ptr<CompositeType>> tags_;
};

}   /* namespace nuocc */
