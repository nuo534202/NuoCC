#include "nuocc_driver.hpp"

#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <memory>

#include "nuocc_asm_codegen.hpp"
#include "nuocc_parser.hpp"
#include "nuocc_scanner.hpp"
#include "utils/nuocc_command.hpp"
#include "utils/nuocc_print.hpp"

/*
 * The C compiler which is asked to assemble and to link, and the runtime
 * the generated programs are linked with. Both are fixed when the
 * compiler is built; the runtime holds printint() and printchar(), which
 * the language may call but does not define itself.
 */
#ifndef NUOCC_CC
#define NUOCC_CC "cc"
#endif

#ifndef NUOCC_RUNTIME
#define NUOCC_RUNTIME "lib/printint.c"
#endif

/*
 * The C pre-processor the source of a program is put through before it is
 * scanned, and the directory our own header files are found in. The
 * pre-processor is told not to look in the system's include directory,
 * because those header files use parts of C this compiler does not have.
 */
#ifndef NUOCC_CPP
#define NUOCC_CPP "cpp"
#endif

#ifndef NUOCC_INCLUDE_DIR
#define NUOCC_INCLUDE_DIR "lib/include"
#endif

namespace nuocc
{

namespace
{

/*
 * The name of the C compiler to assemble and to link with. The CC
 * environment variable names another one, the same way a makefile reads
 * it.
 */
std::string CompilerName()
{
    const char *named = std::getenv("CC");

    if (named != nullptr && named[0] != '\0')
        return named;

    return NUOCC_CC;
}

/* The file name without the directories it is found in. */
std::string FileName(const std::string& path)
{
    const size_t slash = path.find_last_of('/');

    if (slash == std::string::npos)
        return path;

    return path.substr(slash + 1);
}

/*
 * The file name with its suffix replaced, or an empty name when it has no
 * suffix to replace. The result has no directory: what a compilation
 * produces is written where the compiler was run, the way the other
 * compilers do it, and not into the directory the source is read from.
 */
std::string ReplaceSuffix(const std::string& path, const std::string& suffix)
{
    std::string name = FileName(path);
    const size_t dot = name.find_last_of('.');

    if (dot == std::string::npos)
        return "";

    return name.substr(0, dot) + suffix;
}

}   /* namespace */

void Driver::Usage(const char* program) const
{
    std::cerr << "Usage: " << program << " [-vcSTM] [-o outfile] file ...";
    std::cerr << std::endl;
    std::cerr << "       -v give verbose output of the compilation stages";
    std::cerr << std::endl;
    std::cerr << "       -c generate object files but do not link them";
    std::cerr << std::endl;
    std::cerr << "       -S stop at the code the target writes, but do not";
    std::cerr << " link" << std::endl;
    std::cerr << "       -T dump the tokens and the tree of each input file";
    std::cerr << std::endl;
    std::cerr << "       -M dump the names each input file declared";
    std::cerr << std::endl;
    std::cerr << "       -o outfile, produce the outfile executable file";
    std::cerr << std::endl;
    std::cerr << "       --target, say which target this compiler was built";
    std::cerr << " for and stop" << std::endl;
}

bool Driver::ParseOptions(int argc, const char* const* argv)
{
    int i = 1;

    /* The options come first, and the files follow them. */
    for (; i < argc; i++)
    {
        const std::string argument = argv[i];

        if (argument.size() < 2 || argument[0] != '-')
            break;

        /*
         * A question about the compiler itself, which is answered and
         * leaves: no input file is needed and nothing is compiled.
         */
        if (argument == "--target")
        {
            std::cout << TargetName() << std::endl;
            std::exit(0);
        }

        for (size_t k = 1; k < argument.size(); k++)
        {
            switch (argument[k])
            {
                case 'o':
                    if (i + 1 == argc)
                    {
                        std::cerr << "error: -o needs a file name after it";
                        std::cerr << std::endl;
                        return false;
                    }

                    options_.output = argv[++i];
                    break;

                case 'c':
                    options_.assemble = true;
                    options_.link = false;
                    break;

                case 'S':
                    options_.assemble = false;
                    options_.link = false;
                    break;

                case 'v':
                    options_.verbose = true;
                    break;

                case 'T':
                    options_.dump = true;
                    break;

                case 'M':
                    options_.symbols = true;
                    break;

                default:
                    std::cerr << "error: unknown option -" << argument[k];
                    std::cerr << std::endl;
                    return false;
            }

            /* -o took the next argument with it. */
            if (argument[k] == 'o')
                break;
        }
    }

    for (; i < argc; i++)
        options_.inputs.push_back(argv[i]);

    if (options_.inputs.empty())
    {
        std::cerr << "error: no input file" << std::endl;
        return false;
    }

    return true;
}

/*
 * Run the pre-processor over one source file and answer with everything
 * it wrote. Nothing comes back when it could not be run or stopped on an
 * error, in which case it has already said why on the error stream.
 */
std::optional<std::string> Driver::Preprocess(const std::string& source) const
{
    /*
     * The system's include directory is left out, since the header files
     * there use parts of C this compiler does not have, and our own is
     * named instead. The two words are joined so that the pre-processor
     * reads one argument however the path is written.
     */
    const std::string command = std::string(NUOCC_CPP) +
        " -nostdinc -I" + Quote(NUOCC_INCLUDE_DIR) + " " + Quote(source);

    if (options_.verbose)
        std::cout << command << std::endl;

    FILE *pipe = popen(command.c_str(), "r");

    if (pipe == nullptr)
    {
        std::cerr << "error: could not run " << NUOCC_CPP << std::endl;
        return std::nullopt;
    }

    std::string text;
    char buffer[4096];
    size_t got;

    while ((got = fread(buffer, 1, sizeof(buffer), pipe)) > 0)
        text.append(buffer, got);

    /*
     * A pre-processor which stopped on an error leaves what it managed to
     * write, which is not the whole program, so it is thrown away.
     */
    if (pclose(pipe) != 0)
    {
        std::cerr << "error: " << source << " could not be pre-processed";
        std::cerr << std::endl;
        return std::nullopt;
    }

    return text;
}

/*
 * Compile one source file down to the code this target writes, and print
 * what was read and what it was made into when -T was given. An error in
 * the source is reported and ends the compilation from inside the compiler,
 * so false only says that the pre-processor could not be run.
 */
bool Driver::CompileToCode(const std::string& source,
    const std::string& output) const
{
    std::optional<std::string> text = Preprocess(source);

    if (!text)
        return false;

    Scanner scanner;
    scanner.Scan(*text);

    if (options_.dump)
        PrintTokenList(scanner);

    Parser parser;
    Program program = parser.Parse(scanner.GetTokenList());

    if (options_.dump)
        PrintSyntaxTree(program);

    if (options_.symbols)
        PrintSymbolTable(parser.GetSymbolTable(), source);

    std::unique_ptr<AsmCodegen> codegen = MakeCodegen(output);
    codegen->GenProgram(program);

    /*
     * The file the code was written into is closed before anything is done
     * with it, and what is done with it is the pass the target has over the
     * whole of the code it wrote: the peephole optimiser, with the rules of
     * the machine it is for, or nothing at all for a target which has no
     * such pass.
     */
    codegen.reset();

    return OptimiseCode(output);
}

bool Driver::RunCommand(const std::string& command) const
{
    if (options_.verbose)
        std::cout << command << std::endl;

    return std::system(command.c_str()) == 0;
}

bool Driver::Assemble(const std::string& assembly,
    const std::string& object) const
{
    return RunCommand(CompilerName() + " -c " + Quote(assembly) + " -o " +
                      Quote(object));
}

/*
 * Link the object files of the program, together with the runtime the
 * generated code calls into. The runtime is only added when it is there,
 * so that a compiler which was moved away from its sources can still link
 * a program which does not need it.
 */
bool Driver::Link(const std::vector<std::string>& objects) const
{
    std::string command = CompilerName() + " -o " + Quote(options_.output);

    for (const std::string& object : objects)
        command += " " + Quote(object);

    const std::filesystem::path runtime(NUOCC_RUNTIME);

    if (std::filesystem::exists(runtime))
    {
        command += " " + Quote(runtime.string());
    }
    else
    {
        std::cerr << "warning: the runtime " << NUOCC_RUNTIME;
        std::cerr << " was not found, so printint() and printchar() are";
        std::cerr << " not linked in" << std::endl;
    }

    return RunCommand(command);
}

/*
 * Make the program out of the file of code with the tool of a target which
 * has one of its own. Such a target makes one program out of one source
 * file, and what it makes is the program: there is nothing to assemble and
 * nothing to link afterwards.
 */
int Driver::BuildProgram(const std::string& builder,
    const std::string& suffix) const
{
    if (options_.assemble)
    {
        std::cerr << "error: -c makes an object file, which this target";
        std::cerr << " does not" << std::endl;
        return 1;
    }

    std::vector<std::string> written;

    for (const std::string& source : options_.inputs)
    {
        const std::string code = ReplaceSuffix(source, suffix);

        if (code.empty())
        {
            std::cerr << "error: " << source << " has no suffix to replace";
            std::cerr << std::endl;
            return 1;
        }

        if (options_.verbose)
            std::cout << "compiling " << source << std::endl;

        if (!CompileToCode(source, code))
            return 1;

        written.push_back(code);
    }

    /* -S asks for the code itself, and stops there. */
    if (!options_.assemble && !options_.link)
        return 0;

    if (!RunCommand(BuildCommand(written, options_.output)))
    {
        std::cerr << builder << " could not build the program" << std::endl;
        return 1;
    }

    /* The code has done its work once it is the program. */
    for (const std::string& code : written)
        std::remove(code.c_str());

    return 0;
}

int Driver::Run(int argc, const char* const* argv)
{
    if (!ParseOptions(argc, argv))
    {
        Usage(argv[0]);
        return 1;
    }

    std::vector<std::string> objects;

    /*
     * What the target of this build writes, the tool which turns it into
     * assembly when it does not write the assembly itself, and the tool
     * which makes the program when the C compiler of the machine the
     * compiler runs on does not.
     */
    const std::string code_suffix = CodeSuffix();
    const std::string code_tool = CodeTool();
    const std::string code_builder = CodeBuilder();

    /*
     * A target which makes the program with a tool of its own makes one
     * program out of one source file: what it writes is not assembled and
     * linked the way the code of the other targets is.
     */
    if (!code_builder.empty())
        return BuildProgram(code_builder, code_suffix);

    for (const std::string& source : options_.inputs)
    {
        const std::string code = ReplaceSuffix(source, code_suffix);

        if (code.empty())
        {
            std::cerr << "error: " << source << " has no suffix to replace";
            std::cerr << std::endl;
            return 1;
        }

        if (options_.verbose)
            std::cout << "compiling " << source << std::endl;

        if (!CompileToCode(source, code))
            return 1;

        /* -S asks for the code itself, and stops there. */
        if (!options_.assemble && !options_.link)
            continue;

        /*
         * A target which writes assembly itself has nothing to turn into
         * it, and what it wrote is the assembly.
         */
        std::string assembly = code;

        if (!code_tool.empty())
        {
            assembly = ReplaceSuffix(source, ".s");

            if (!RunCommand(code_tool + " -o " + Quote(assembly) + " " +
                            Quote(code)))
            {
                std::cerr << code_tool << " could not compile " << code;
                std::cerr << std::endl;
                return 1;
            }
        }

        const std::string object = ReplaceSuffix(source, ".o");

        if (!Assemble(assembly, object))
        {
            std::cerr << "Assembly of " << assembly << " failed";
            std::cerr << std::endl;
            return 1;
        }

        /* The code the compiler wrote has done its work once it is an
         * object file, and so has the assembly it was turned into. */
        std::remove(code.c_str());

        if (assembly != code)
            std::remove(assembly.c_str());

        objects.push_back(object);
    }

    if (options_.link)
    {
        if (!Link(objects))
        {
            std::cerr << "Linking failed" << std::endl;
            return 1;
        }

        /*
         * The object files were made so that the program could be linked,
         * and are taken away once it has been. Keeping them is what the
         * -c flag asks for.
         */
        for (const std::string& object : objects)
            std::remove(object.c_str());
    }

    return 0;
}

}   /* namespace nuocc */
