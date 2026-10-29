#include "nuocc_driver.hpp"

#include <cctype>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <memory>
#include <utility>

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

struct ToolWords
{
    std::vector<std::string> arguments;
    std::string error;
};

/*
 * Split a configured command into words using quoting and backslash escapes,
 * without asking a shell to expand variables, globs, or command substitutions.
 */
ToolWords SplitWords(const std::string& value)
{
    ToolWords result;
    std::string word;
    char quote = '\0';
    bool started = false;

    for (std::size_t index = 0; index < value.size(); ++index)
    {
        const char character = value[index];
        if (quote == '\'')
        {
            if (character == '\'')
                quote = '\0';
            else
                word += character;
            continue;
        }

        if (character == '\\' && index + 1 < value.size() &&
            (value[index + 1] == '\n' ||
                (value[index + 1] == '\r' && index + 2 < value.size() &&
                    value[index + 2] == '\n')))
        {
            index += value[index + 1] == '\n' ? 1 : 2;
            continue;
        }
        if (quote == '"')
        {
            if (character == '"')
            {
                quote = '\0';
            }
            else if (character == '\\' && index + 1 < value.size() &&
                (value[index + 1] == '"' || value[index + 1] == '\\'))
            {
                word += value[++index];
            }
            else
            {
                word += character;
            }
            continue;
        }

        if (std::isspace(static_cast<unsigned char>(character)) != 0)
        {
            if (started)
            {
                result.arguments.push_back(std::move(word));
                word.clear();
                started = false;
            }
        }
        else if (character == '\'' || character == '"')
        {
            quote = character;
            started = true;
        }
        else if (character == '\\')
        {
            if (index + 1 == value.size())
            {
                result.error = "the tool command ends with an escape";
                return result;
            }
            word += value[++index];
            started = true;
        }
        else
        {
            word += character;
            started = true;
        }
    }

    if (quote != '\0')
    {
        result.error = "the tool command has an unterminated quote";
        return result;
    }
    if (started)
        result.arguments.push_back(std::move(word));
    if (result.arguments.empty())
        result.error = "the tool command is empty";
    return result;
}

std::string CompilerValue()
{
    const char* named = std::getenv("CC");

    if (named == nullptr || named[0] == '\0')
        return NUOCC_CC;
    return named;
}

bool ResolveTool(const std::string& value, const std::string& tool_name,
    std::vector<std::string>& command)
{
    ToolWords words = SplitWords(value);
    if (!words.error.empty())
    {
        std::cerr << "error: invalid " << tool_name << ": ";
        std::cerr << words.error << std::endl;
        return false;
    }
    if (words.arguments.front().empty())
    {
        std::cerr << "error: invalid " << tool_name;
        std::cerr << ": the executable is empty" << std::endl;
        return false;
    }

    command = std::move(words.arguments);
    return true;
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

bool InspectFile(const std::string& path, bool& exists)
{
    std::error_code error;
    exists = std::filesystem::exists(path, error);
    if (!error)
        return true;

    std::cerr << "error: could not inspect " << path << ": ";
    std::cerr << error.message() << std::endl;
    return false;
}

void ReportToolFailure(const ToolCommand& command, const ToolResult& result)
{
    std::cerr << "error: " << FormatCommand(command);
    if (!result.error_message.empty())
        std::cerr << ": " << result.error_message;
    else if (result.exited)
        std::cerr << ": exited with status " << result.exit_status;
    std::cerr << std::endl;

    if (result.standard_error && !result.standard_error->empty())
    {
        std::cerr << *result.standard_error;
        if (result.standard_error->back() != '\n')
            std::cerr << std::endl;
    }
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
std::optional<std::string> Driver::Preprocess(const std::string& source,
    const std::vector<std::string>& cpp_command) const
{
    /*
     * The system's include directory is left out, since the header files
     * there use parts of C this compiler does not have, and our own is
     * named instead. The include path is one argument, so the pre-processor
     * sees it whole even when it contains spaces or shell metacharacters.
     */
    std::vector<std::string> arguments = cpp_command;
    arguments.insert(arguments.end(), {"-nostdinc",
        "-I" + std::string(NUOCC_INCLUDE_DIR), source});
    const ToolCommand command{
        .arguments = std::move(arguments),
        .input_files = {}};

    if (options_.verbose)
        std::cout << FormatCommand(command) << std::endl;

    ToolResult result = RunTool(command, true);

    if (!result.Succeeded())
    {
        ReportToolFailure(command, result);
        std::cerr << "error: " << source << " could not be pre-processed";
        std::cerr << std::endl;
        return std::nullopt;
    }

    if (result.standard_error && !result.standard_error->empty())
        std::cerr << *result.standard_error;
    return std::move(*result.standard_output);
}

/*
 * Compile one source file down to the code this target writes, and print
 * what was read and what it was made into when -T was given. An error in
 * the source is reported and ends the compilation from inside the compiler,
 * so false only says that the pre-processor could not be run.
 */
bool Driver::CompileToCode(const std::string& source,
    const std::string& output,
    const std::vector<std::string>& cpp_command) const
{
    std::optional<std::string> text = Preprocess(source, cpp_command);

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

bool Driver::RunCommand(const ToolCommand& command) const
{
    if (options_.verbose)
        std::cout << FormatCommand(command) << std::endl;

    ToolResult result = RunTool(command, false);
    if (!result.Succeeded())
    {
        ReportToolFailure(command, result);
        return false;
    }
    return true;
}

bool Driver::Assemble(const std::string& assembly,
    const std::string& object,
    const std::vector<std::string>& compiler_command) const
{
    std::vector<std::string> arguments = compiler_command;
    arguments.insert(arguments.end(), {"-c", assembly, "-o", object});
    return RunCommand({
        .arguments = std::move(arguments), .input_files = {}});
}

/*
 * Link the object files of the program, together with the runtime the
 * generated code calls into. The runtime is only added when it is there,
 * so that a compiler which was moved away from its sources can still link
 * a program which does not need it.
 */
bool Driver::Link(const std::vector<std::string>& objects,
    const std::vector<std::string>& compiler_command) const
{
    std::vector<std::string> arguments = compiler_command;
    arguments.insert(arguments.end(), {"-o", options_.output});

    for (const std::string& object : objects)
        arguments.push_back(object);

    const std::filesystem::path runtime(NUOCC_RUNTIME);
    std::error_code runtime_error;

    if (std::filesystem::exists(runtime, runtime_error))
    {
        arguments.push_back(runtime.string());
    }
    else if (runtime_error)
    {
        std::cerr << "error: could not inspect " << NUOCC_RUNTIME << ": "
                  << runtime_error.message() << std::endl;
        return false;
    }
    else
    {
        std::cerr << "warning: the runtime " << NUOCC_RUNTIME;
        std::cerr << " was not found, so printint() and printchar() are";
        std::cerr << " not linked in" << std::endl;
    }

    return RunCommand({
        .arguments = std::move(arguments), .input_files = {}});
}

/*
 * Make the program out of the file of code with the tool of a target which
 * has one of its own. Such a target makes one program out of one source
 * file, and what it makes is the program: there is nothing to assemble and
 * nothing to link afterwards.
 */
bool Driver::BuildProgram(const std::string& suffix,
    const std::vector<std::string>& cpp_command) const
{
    if (options_.assemble)
    {
        std::cerr << "error: -c makes an object file, which this target";
        std::cerr << " does not" << std::endl;
        return false;
    }

    std::vector<std::string> written;
    std::vector<std::string> created;
    const auto remove_created = [&created]()
    {
        for (const std::string& code : created)
            std::remove(code.c_str());
    };

    for (const std::string& source : options_.inputs)
    {
        const std::string code = ReplaceSuffix(source, suffix);
        if (code.empty())
        {
            std::cerr << "error: " << source << " has no suffix to replace";
            std::cerr << std::endl;
            remove_created();
            return false;
        }

        if (options_.verbose)
            std::cout << "compiling " << source << std::endl;

        bool existed = false;
        if (!InspectFile(code, existed))
        {
            remove_created();
            return false;
        }
        if (!CompileToCode(source, code, cpp_command))
        {
            bool created_now = false;
            if (!InspectFile(code, created_now))
            {
                remove_created();
                return false;
            }
            if (!existed && created_now)
                created.push_back(code);
            remove_created();
            return false;
        }
        written.push_back(code);
        if (!existed)
            created.push_back(code);
    }

    if (!options_.link)
        return true;

    const std::optional<ToolCommand> command =
        BuildCommand(written, options_.output);
    if (!command || command->arguments.empty())
    {
        std::cerr << "error: this target cannot build a program" << std::endl;
        remove_created();
        return false;
    }
    if (!RunCommand(*command))
    {
        remove_created();
        return false;
    }

    remove_created();
    return true;
}

int Driver::Run(int argc, const char* const* argv)
{
    if (!ParseOptions(argc, argv))
    {
        Usage(argv[0]);
        return 1;
    }

    std::vector<std::string> objects;
    std::vector<std::string> created_objects;
    std::vector<std::string> cpp_command;
    if (!ResolveTool(NUOCC_CPP, "preprocessor", cpp_command))
        return 1;

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
        return BuildProgram(code_suffix, cpp_command) ? 0 : 1;

    std::vector<std::string> compiler_command;
    if ((options_.assemble || options_.link) &&
        !ResolveTool(CompilerValue(), "compiler", compiler_command))
    {
        return 1;
    }

    std::vector<std::string> code_tool_command;
    if (!code_tool.empty() && (options_.assemble || options_.link) &&
        !ResolveTool(code_tool, "target code tool", code_tool_command))
    {
        return 1;
    }

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

        bool code_existed = false;
        if (!InspectFile(code, code_existed))
            return 1;
        if (!CompileToCode(source, code, cpp_command))
            return 1;

        /* -S asks for the code itself, and stops there. */
        if (!options_.assemble && !options_.link)
            continue;

        /*
         * A target which writes assembly itself has nothing to turn into
         * it, and what it wrote is the assembly.
         */
        std::string assembly = code;
        bool assembly_existed = code_existed;

        if (!code_tool.empty())
        {
            assembly = ReplaceSuffix(source, ".s");
            if (!InspectFile(assembly, assembly_existed))
                return 1;
            std::vector<std::string> arguments = code_tool_command;
            arguments.insert(arguments.end(), {"-o", assembly, code});

            if (!RunCommand({
                    .arguments = std::move(arguments),
                    .input_files = {}}))
            {
                return 1;
            }
        }

        const std::string object = ReplaceSuffix(source, ".o");
        bool object_existed = false;
        if (!InspectFile(object, object_existed) ||
            !Assemble(assembly, object, compiler_command))
        {
            return 1;
        }

        /* The code the compiler wrote has done its work once it is an
         * object file, and so has the assembly it was turned into. */
        if (!code_existed)
            std::remove(code.c_str());

        if (assembly != code && !assembly_existed)
            std::remove(assembly.c_str());

        objects.push_back(object);
        if (!object_existed)
            created_objects.push_back(object);
    }

    if (options_.link)
    {
        if (!Link(objects, compiler_command))
            return 1;

        /*
         * The object files were made so that the program could be linked,
         * and are taken away once it has been. Keeping them is what the
         * -c flag asks for.
         */
        for (const std::string& object : created_objects)
            std::remove(object.c_str());
    }

    return 0;
}

}   /* namespace nuocc */
