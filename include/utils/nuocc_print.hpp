#pragma once

#include <memory>
#include <string>

#include "nodes/nuocc_nodes_tag.hpp"
#include "nodes/nuocc_scanner_nodes.hpp"
#include "nuocc_scanner.hpp"

namespace nuocc
{

/*
 * A readable name for a node tag, used by the diagnostics. It is always
 * compiled in, a syntax error in a release build has to be as readable as
 * one in a debug build.
 */
std::string NodeTagToString(const NodeTag& tag);

#ifdef NDEBUG

/* Release Mode */
#define PRINTTOKENLIST(scanner) ((void)0)

#else

/* Debug Mode */
#define PRINTTOKENLIST(scanner) PrintTokenList(scanner)

void PrintTokenList(const Scanner& scanner);

#endif

}   /* namespace nuocc */
