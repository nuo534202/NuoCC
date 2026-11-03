#include <cstdlib>
#include <exception>
#include <iostream>
#include <string>
#include <string_view>

#include "nuocc_scanner.hpp"
#include "utils/nuocc_error.hpp"

namespace
{

struct DestructionProbe
{
    bool& destroyed;

    ~DestructionProbe()
    {
        destroyed = true;
    }
};

struct ErrorCase
{
    std::string_view input;
    std::string_view diagnostic;
};

}   /* namespace */

int main()
{
    constexpr ErrorCase cases[] = {
        {"@", "Error: unrecognized character @!"},
        {"'", "lexical error: unterminated character literal!"},
        {"'ab'", "lexical error: expected ' at end of character literal!"},
        {"\"abc", "lexical error: unterminated string literal!"},
        {R"("abc\)",
         "lexical error: expected a character after a backslash!"},
        {R"("\q")", "lexical error: unknown escape sequence \\q!"},
        {R"("\x100")",
         "lexical error: the character code 256 is too large for a char!"},
        {R"("\x")", "lexical error: there are no digits in the character code!"},
        {"..", "lexical error: unknown token!"},
        {"0x", "lexical error: there are no digits after the 0x of 0x!"},
        {"0999", "lexical error: the digit 9 of 0999 is not one a number of "
                 "radix 8 holds!"},
        {"2147483648", "lexical error: integer literal 2147483648 is too large!"},
    };

    for (const ErrorCase& test_case : cases)
    {
        bool destroyed = false;
        try
        {
            DestructionProbe probe{destroyed};
            nuocc::Scanner scanner;
            scanner.Scan(std::string(test_case.input));
        }
        catch (const nuocc::CompilerError& error)
        {
            if (!destroyed || error.what() != test_case.diagnostic)
            {
                std::cerr << "scanner failure did not preserve its diagnostic "
                             "and unwind its scope: " << test_case.input
                          << ": " << error.what() << std::endl;
                return EXIT_FAILURE;
            }
            continue;
        }
        catch (const std::exception& error)
        {
            std::cerr << "scanner threw the wrong exception: " << error.what()
                      << std::endl;
            return EXIT_FAILURE;
        }

        std::cerr << "scanner accepted invalid input: " << test_case.input
                  << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
