#pragma once

#include <string>

namespace nuocc
{

/*
 * A word of a command as a shell has to see it, so that a path with a space
 * in it still reaches the tool as one word.
 */
std::string Quote(const std::string& text);

}   /* namespace nuocc */
