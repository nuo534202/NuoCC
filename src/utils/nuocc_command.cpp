#include "utils/nuocc_command.hpp"

namespace nuocc
{

std::string Quote(const std::string& text)
{
    return "'" + text + "'";
}

}   /* namespace nuocc */
