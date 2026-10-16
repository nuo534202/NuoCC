#pragma once

#include <optional>
#include <string>
#include <vector>

namespace nuocc
{

/*
 * The command line of the compiler and the stages it puts an input file
 * through: the source is compiled to the code the target writes, that code
 * is turned into assembly when the target does not write it itself, and the
 * assembly is handed to the system assembler and linker unless the flags
 * say to stop.
 */
class Driver
{
public:
    /*
     * Do what the command line asks for and answer with the status the
     * program exits with: zero when everything asked for was done.
     */
    int Run(int argc, const char* const* argv);

private:
    /* What the command line asked for. */
    struct Options
    {
        /* -v, print the stage of the compilation being run. */
        bool verbose = false;
        /* -T, print the tokens and the tree of each input file. */
        bool dump = false;
        /* -M, print the names each input file declared. */
        bool symbols = false;
        /* -c, assemble the input files but do not link them. */
        bool assemble = false;
        /* Whether to link the object files into an executable. */
        bool link = true;
        /* --target, say which target this compiler was built for. */
        bool target = false;
        /* -o, the file to link the program into. */
        std::string output = "a.out";
        /* The source files named on the command line. */
        std::vector<std::string> inputs;
    };

    /* Read the options, which come before any file name. */
    bool ParseOptions(int argc, const char* const* argv);
    void Usage(const char* program) const;

    /* The stages of the compilation. */

    /*
     * Make the program out of the code a target which builds its own
     * programs wrote, which is one file of code for each source file.
     */
    bool CompileToCode(const std::string& source,
                       const std::string& output) const;

    /*
     * Make the program out of the code a target which builds its own
     * programs wrote, which is one file of code for each source file.
     */
    int BuildProgram(const std::string& builder,
                     const std::string& suffix) const;

    /*
     * Feed one source file through the C pre-processor and answer with
     * what it wrote, or nothing when it could not be pre-processed. The
     * pre-processor is what expands the #include and #define lines and
     * takes the comments away, so the scanner which follows only ever
     * sees plain declarations.
     */
    std::optional<std::string> Preprocess(const std::string& source) const;
    bool Assemble(const std::string& assembly,
                  const std::string& object) const;
    bool Link(const std::vector<std::string>& objects) const;

    /* Run one command, having said what it is when -v was given. */
    bool RunCommand(const std::string& command) const;

    Options options_;
};

}   /* namespace nuocc */
