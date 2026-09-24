#include <iostream>
#include <string>

#include "nuocc_parser.hpp"
#include "nuocc_scanner.hpp"
#include "utils/nuocc_print.hpp"

int main(int argc, const char **argv)
{
    if (argc < 2)
    {
        std::cerr << "usage: " << argv[0] << " <input file>" << std::endl;
        return 1;
    }

    std::string file(argv[1]);
    std::cout << file << std::endl;

    nuocc::Scanner scanner;
    scanner.Scan(file);
    nuocc::PRINTTOKENLIST(scanner);

    nuocc::Parser parser;
    parser.Parse(scanner.GetTokenList());

    return 0;
}