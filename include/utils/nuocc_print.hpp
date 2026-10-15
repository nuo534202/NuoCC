#pragma once

#include <memory>
#include <string>

#include "nodes/nuocc_nodes_tag.hpp"
#include "nodes/nuocc_scanner_nodes.hpp"
#include "nuocc_scanner.hpp"
#include "utils/nuocc_symbol_table.hpp"
#include "utils/nuocc_types.hpp"

namespace nuocc
{

/*
 * A readable name for a node tag, used by the diagnostics. It is always
 * compiled in, a syntax error in a release build has to be as readable as
 * one in a debug build.
 */
std::string NodeTagToString(const NodeTag& tag);

/*
 * Print the tokens the scanner read, one per line, which is what the -T
 * flag asks for. It is always compiled in: what is printed is asked for
 * on the command line rather than decided when the compiler is built.
 */
void PrintTokenList(const Scanner& scanner);

/*
 * Print the whole program as a tree, one node per line, indented by how
 * deep the node stands. The storage of the globals is printed first, then
 * each function, the same order the code generator walks them in.
 */
void PrintSyntaxTree(const Program& program);

/*
 * Print every name the symbol table holds, which is what the -M flag asks
 * for. The names are printed after the whole file has been read, so the
 * table holds the globals, the functions, the typedefs and the enum values
 * the file declared, and none of the locals of any of its functions.
 */
void PrintSymbolTable(const SymbolTable& table, const std::string& source);

}   /* namespace nuocc */
