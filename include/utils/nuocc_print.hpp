#pragma once

#include <iostream>
#include <memory>
#include <string>

#include "nodes/nuocc_nodes_tag.hpp"
#include "nodes/nuocc_scanner_nodes.hpp"
#include "nuocc_scanner.hpp"

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