#include "nuocc_driver.hpp"

int main(int argc, const char **argv)
{
    nuocc::Driver driver;

    return driver.Run(argc, argv);
}
