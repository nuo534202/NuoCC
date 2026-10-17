#include "codegen/nuocc_codegen_assembly.hpp"

#include <stdlib.h>

#include <iostream>

namespace nuocc
{

AssemblyCodegen::AssemblyCodegen(const std::string& output_file)
    : AsmCodegen(output_file)
{
}

/*
 * The storage of a global variable begins in the data section, where the
 * assembler is told which name other files may reach and left to itself
 * about one declared static, which only this file may.
 */
/*
 * The code and the storage of a program of this target are kept apart by
 * the assembler, so nothing waits for the end of the program.
 */
void AssemblyCodegen::EmitEndOfProgram()
{
}

void AssemblyCodegen::BeginGlobal(const Symbol& symbol, int32 alignment)
{
    ofs_ << "\t.data" << std::endl;

    if (!symbol.is_static)
        ofs_ << "\t.globl\t" << GlobName(symbol.name) << std::endl;

    /*
     * The storage is lined up to what it holds, so that every element of it
     * lands where its type says and so that a pointer an element holds can
     * be written into it. The data section of some machines lines a pointer
     * up no further than this whatever the assembler is told, which is why
     * the alignment is asked for here rather than left to them.
     */
    if (alignment > 1)
        ofs_ << "\t.balign\t" << alignment << std::endl;

    ofs_ << GlobName(symbol.name) << ":" << std::endl;
}

/*
 * The room a struct or a union takes is one block of it, which starts out
 * as zeroes however much of it the program goes on to write.
 */
void AssemblyCodegen::EmitGlobalSpace(int32 size)
{
    ofs_ << "\t.space\t" << size << std::endl;
}

/*
 * One element of a global variable, held by the directive of its size.
 *
 * The directive is named for the number of bytes it holds rather than for
 * the word of a machine, since the assemblers these targets are written
 * with do not agree on how wide their word is: a two byte element would
 * take four bytes of storage on the 64 bit arm machine, and the element
 * after it would stand in the wrong place.
 */
void AssemblyCodegen::EmitGlobalValue(int32 value, int32 size)
{
    switch (size)
    {
        case 1:
            ofs_ << "\t.byte\t" << value << std::endl;
            return;
        case 2:
            ofs_ << "\t.short\t" << value << std::endl;
            return;
        case 4:
            ofs_ << "\t.long\t" << value << std::endl;
            return;
        case 8:
            ofs_ << "\t.quad\t" << value << std::endl;
            return;
        default:
            break;
    }

    std::cerr << "Error: a global element of " << size;
    std::cerr << " bytes has no directive!" << std::endl;
    std::exit(1);
}

/*
 * An element which holds the address of a string holds the address the
 * label of that string stands for.
 */
void AssemblyCodegen::EmitGlobalLabelRef(label_idx label)
{
    ofs_ << "\t.quad\tL" << label << std::endl;
}

/* The code of the program follows the storage of a global variable. */
void AssemblyCodegen::EndGlobal()
{
    ofs_ << "\t.text" << std::endl;
}

/*
 * The storage of a string holds one byte per character and then the NUL
 * which ends the string, all under a local label: unlike a global variable,
 * a literal has no name the rest of the program could refer to.
 */
void AssemblyCodegen::GenStrStorage(label_idx label, const std::string& text)
{
    ofs_ << "\t.data" << std::endl;
    EmitLabel(label);

    ofs_ << "\t.byte\t";

    for (std::size_t k = 0; k < text.size(); k++)
    {
        if (k > 0)
            ofs_ << ", ";

        ofs_ << static_cast<int>(static_cast<unsigned char>(text[k]));
    }

    if (!text.empty())
        ofs_ << ", ";

    ofs_ << "0" << std::endl;
    ofs_ << "\t.text" << std::endl;
}

/*
 * The machine targets write the assembly of the machine themselves, so what
 * they write is the file the assembler is given and no tool is needed to
 * turn it into one.
 */
std::string CodeSuffix()
{
    return ".s";
}

std::string CodeTool()
{
    return "";
}

/* The program is made by the C compiler of the machine the compiler runs
 * on, which assembles the code and links the runtime with it. */
std::string CodeBuilder()
{
    return "";
}

std::string BuildCommand(const std::vector<std::string>& /*code*/,
    const std::string& /*program*/)
{
    return "";
}

}   /* namespace nuocc */
