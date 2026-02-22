#include <iostream>
#include <string>

#include "nuocc_asm_codegen.hpp"
#include "nuocc_parser.hpp"
#include "nuocc_scanner.hpp"
#include "utils/nuocc_print.hpp"

int main(int argc, const char **argv)
{
    if (argc < 2)
    {
        std::cerr << "wrong input type" << std::endl;
    }

    std::string file(argv[1]);
    std::cout << file << std::endl;

    nuocc::Scanner scanner;
    scanner.Scan(file);
    nuocc::PRINTTOKENLIST(scanner);

    nuocc::Parser parser;
    parser.Parse(scanner.GetTokenList());

    std::string output_file("asm_out.txt");
    nuocc::AsmCodegen asm_codegen(output_file);
    asm_codegen.GenerateAsmCode(parser.GetAstRoot());

    return 0;
}