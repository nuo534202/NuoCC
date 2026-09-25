#include <iostream>
#include <memory>
#include <string>
#include <vector>

#include "nuocc_asm_codegen.hpp"
#include "nuocc_parser.hpp"
#include "nuocc_scanner.hpp"
#include "utils/nuocc_print.hpp"

namespace
{

constexpr char kOutputFile[] = "asm_out.txt";

}   /* namespace */

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
    std::vector<nuocc::AstNodePtr> program
        = parser.Parse(scanner.GetTokenList());

    std::unique_ptr<nuocc::AsmCodegen> asm_codegen
        = nuocc::MakeCodegen(kOutputFile);

    asm_codegen->GenProgram(program);

    return 0;
}
