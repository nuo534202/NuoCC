#include "nuocc_driver.hpp"

#include <exception>
#include <iostream>

#include "utils/nuocc_error.hpp"

int main(int argc, const char **argv)
{
    try
    {
        nuocc::Driver driver;

        return driver.Run(argc, argv);
    }
    catch (const nuocc::CompilerError& error)
    {
        std::cerr << error.what() << '\n';
        return 1;
    }
    catch (const std::exception& error)
    {
        std::cerr << "error: " << error.what() << '\n';
        return 1;
    }
    catch (...)
    {
        std::cerr << "error: unexpected compiler failure\n";
        return 1;
    }
}
