#include <iostream>
#include <string>

#include "scanner.hpp"
#include "utils/print.hpp"

int main(int argc, const char **argv)
{
    if (argc < 2)
    {
        std::cerr << "wrong input type" << std::endl;
    }

    std::string file(argv[1]);
    std::cout << file << std::endl;

    nuocc::Scanner scanner(file);
    scanner.Scan();
    nuocc::PRINTTOKENLIST(scanner);

    return 0;
}