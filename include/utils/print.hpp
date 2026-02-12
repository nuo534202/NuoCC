#pragma once

#include <iostream>
#include <memory>
#include <string>

#include "nodes/nodes_tag.hpp"
#include "nodes/scanner_nodes.hpp"
#include "scanner.hpp"

#ifdef NDEBUG

/* Release Mode */
namespace nuocc
{

#define PRINTTOKENLIST(scanner) ((void)0)

}   /* namespace nuocc */

#else

/* Debug Mode */
namespace nuocc
{

#define PRINTTOKENLIST(scanner) PrintTokenList(scanner)

void PrintTokenList(const nuocc::Scanner& scanner);
std::string NodeTagToString(const nuocc::NodeTag& tag);

}   /* namespace nuocc */

#endif