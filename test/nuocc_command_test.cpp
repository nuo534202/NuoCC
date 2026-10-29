#include <cerrno>
#include <cstdlib>
#include <exception>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>

#include "utils/nuocc_command.hpp"

namespace
{

class TemporaryDirectory
{
public:
    TemporaryDirectory()
    {
        const std::string pattern = (std::filesystem::temp_directory_path() /
            "nuocc command '; $HOME `test` XXXXXX").string();
        std::vector<char> buffer(pattern.begin(), pattern.end());
        buffer.push_back('\0');
        const char* created = mkdtemp(buffer.data());
        if (created == nullptr)
        {
            throw std::filesystem::filesystem_error("mkdtemp", pattern,
                std::error_code(errno, std::generic_category()));
        }
        path_ = created;
    }

    ~TemporaryDirectory()
    {
        std::error_code error;
        std::filesystem::remove_all(path_, error);
    }

    TemporaryDirectory(const TemporaryDirectory&) = delete;
    TemporaryDirectory& operator=(const TemporaryDirectory&) = delete;

    const std::filesystem::path& GetPath() const
    {
        return path_;
    }

private:
    std::filesystem::path path_;
};

bool WriteFile(const std::filesystem::path& path, const std::string& text)
{
    std::ofstream output(path, std::ios::binary);
    output << text;
    output.close();
    return static_cast<bool>(output);
}

bool CheckQuote()
{
    return nuocc::Quote("") == "''" &&
        nuocc::Quote("a'b") == "'a'\\''b'" &&
        nuocc::FormatCommand({{"/bin/tool", "two words"}, {}}) ==
            "'/bin/tool' 'two words'" &&
        nuocc::FormatCommand({{"cat"}, {"first input", "second input"}}) ==
            "cat 'first input' 'second input' | 'cat'";
}

bool CheckArguments(const std::filesystem::path& directory)
{
    const std::filesystem::path script =
        directory / "tool with '; $HOME `marker`";
    if (!WriteFile(script,
            "#!/bin/sh\n"
            "if [ \"$1\" = \"--exit-seven\" ]; then exit 7; fi\n"
            "if [ \"$1\" = \"--signal\" ]; then kill -TERM \"$$\"; fi\n"
            "if [ \"$1\" = \"--silent\" ]; then exit 0; fi\n"
            "if [ \"$1\" = \"--stderr\" ]; then\n"
            "    printf 'diagnostic\\n' >&2\n"
            "    exit 4\n"
            "fi\n"
            "if [ \"$1\" = \"--ten-bytes\" ]; then\n"
            "    printf '0123456789'\n"
            "    exit 0\n"
            "fi\n"
            "if [ \"$1\" = \"--both-large\" ]; then\n"
            "    printf 'abc'\n"
            "    awk 'BEGIN { for (i = 0; i < 100000; ++i) printf \"x\" }' >&2\n"
            "    exit 0\n"
            "fi\n"
            "if [ \"$1\" = \"--stderr-large\" ]; then\n"
            "    awk 'BEGIN { for (i = 0; i < 100000; ++i) printf \"x\" }' >&2\n"
            "    exit 0\n"
            "fi\n"
            "if [ \"$1\" = \"--hang\" ]; then\n"
            "    while :; do :; done\n"
            "fi\n"
            "if [ \"$1\" = \"--large\" ]; then\n"
            "    awk 'BEGIN { for (i = 0; i < 100000; ++i) printf \"x\" }'\n"
            "    exit 0\n"
            "fi\n"
            "if [ \"$1\" = \"--read-stdin\" ]; then\n"
            "    if ! exec 9<&0; then exit 8; fi\n"
            "    if IFS= read -r line; then exit 9; fi\n"
            "    exit 0\n"
            "fi\n"
            "printf '%s\\n' \"$@\"\n") ||
        chmod(script.c_str(), 0700) != 0)
    {
        std::cerr << "could not create the argument test tool" << std::endl;
        return false;
    }

    const std::vector<std::string> expected_arguments = {
        "", "plain", "two words", "semi;colon", "$HOME",
        "`printf injected`", "single'quote",};
    std::vector<std::string> arguments = {script.string()};
    arguments.insert(arguments.end(), expected_arguments.begin(),
        expected_arguments.end());

    const nuocc::ToolResult arguments_result =
        nuocc::RunTool({.arguments = arguments, .input_files = {}}, true);
    std::string expected_output;
    for (const std::string& argument : expected_arguments)
        expected_output += argument + "\n";
    if (!arguments_result.Succeeded() || !arguments_result.standard_output ||
        *arguments_result.standard_output != expected_output)
    {
        std::cerr << "the tool received different arguments: "
                  << arguments_result.standard_output.value_or("<none>")
                  << " expected: " << expected_output << std::endl;
        return false;
    }

    const nuocc::ToolResult large = nuocc::RunTool(
        {.arguments = {script.string(), "--large"}, .input_files = {}}, true);
    if (!large.Succeeded() || !large.standard_output ||
        *large.standard_output != std::string(100000, 'x'))
    {
        std::cerr << "large captured output was not read completely" << std::endl;
        return false;
    }

    const nuocc::ToolResult limited = nuocc::RunTool(
        {.arguments = {script.string(), "--ten-bytes"}, .input_files = {}},
        true, 5);
    if (limited.error != nuocc::ToolErrorCode::kOutputLimit ||
        !limited.standard_output || *limited.standard_output != "01234")
    {
        std::cerr << "the captured output limit was not enforced" << std::endl;
        return false;
    }

    const nuocc::ToolResult empty_limit = nuocc::RunTool(
        {.arguments = {script.string(), "--ten-bytes"}, .input_files = {}},
        true, 0);
    if (empty_limit.error != nuocc::ToolErrorCode::kOutputLimit ||
        !empty_limit.standard_output || !empty_limit.standard_output->empty())
    {
        std::cerr << "a zero output limit was not enforced" << std::endl;
        return false;
    }

    const nuocc::ToolResult combined_limited = nuocc::RunTool(
        {.arguments = {script.string(), "--both-large"}, .input_files = {}},
        true, 5);
    if (combined_limited.error != nuocc::ToolErrorCode::kOutputLimit ||
        !combined_limited.standard_output ||
        *combined_limited.standard_output != "abc" ||
        !combined_limited.standard_error ||
        *combined_limited.standard_error != "xx")
    {
        std::cerr << "the output limit was not shared by both streams"
                  << std::endl;
        return false;
    }

    const nuocc::ToolResult error_limited = nuocc::RunTool(
        {.arguments = {script.string(), "--stderr-large"}, .input_files = {}},
        true, 5);
    if (error_limited.error != nuocc::ToolErrorCode::kOutputLimit ||
        !error_limited.standard_output ||
        !error_limited.standard_output->empty() ||
        !error_limited.standard_error ||
        *error_limited.standard_error != "xxxxx")
    {
        std::cerr << "the combined output limit did not cover standard error"
                  << std::endl;
        return false;
    }

    const nuocc::ToolResult timed_out = nuocc::RunTool(
        {.arguments = {script.string(), "--hang"}, .input_files = {}}, true,
        nuocc::kDefaultMaximumOutput, std::chrono::milliseconds(20));
    if (timed_out.error != nuocc::ToolErrorCode::kTimeout ||
        !timed_out.standard_output || !timed_out.standard_error)
    {
        std::cerr << "a captured tool timeout was not reported" << std::endl;
        return false;
    }

    const nuocc::ToolResult uncaptured_timeout = nuocc::RunTool(
        {.arguments = {script.string(), "--hang"}, .input_files = {}}, false,
        nuocc::kDefaultMaximumOutput, std::chrono::milliseconds(20));
    if (uncaptured_timeout.error != nuocc::ToolErrorCode::kTimeout)
    {
        std::cerr << "an uncaptured tool timeout was not reported" << std::endl;
        return false;
    }

    const nuocc::ToolResult silent_empty_limit = nuocc::RunTool(
        {.arguments = {script.string(), "--silent"}, .input_files = {}}, true, 0);
    if (!silent_empty_limit.Succeeded() || !silent_empty_limit.standard_output ||
        !silent_empty_limit.standard_output->empty())
    {
        std::cerr << "a zero limit rejected silent output" << std::endl;
        return false;
    }

    const nuocc::ToolResult diagnostic = nuocc::RunTool(
        {.arguments = {script.string(), "--stderr"}, .input_files = {}}, true);
    if (diagnostic.error != nuocc::ToolErrorCode::kNone ||
        diagnostic.exit_status != 4 || !diagnostic.standard_output ||
        !diagnostic.standard_output->empty() || !diagnostic.standard_error ||
        *diagnostic.standard_error != "diagnostic\n")
    {
        std::cerr << "standard error was not captured" << std::endl;
        return false;
    }

    const nuocc::ToolResult failed = nuocc::RunTool(
        {.arguments = {script.string(), "--exit-seven"}, .input_files = {}},
        false);
    if (failed.error != nuocc::ToolErrorCode::kNone || !failed.exited ||
        failed.exit_status != 7 || failed.Succeeded() ||
        failed.standard_output.has_value())
    {
        std::cerr << "a nonzero tool status was not preserved" << std::endl;
        return false;
    }

    const nuocc::ToolResult signaled = nuocc::RunTool(
        {.arguments = {script.string(), "--signal"}, .input_files = {}}, false);
    if (signaled.error != nuocc::ToolErrorCode::kSignal)
    {
        std::cerr << "a signaled tool was not reported" << std::endl;
        return false;
    }

    const std::filesystem::path missing = directory / "missing tool";
    const nuocc::ToolResult not_started = nuocc::RunTool(
        {.arguments = {missing.string()}, .input_files = {}}, false);
    if (not_started.error != nuocc::ToolErrorCode::kStart)
    {
        std::cerr << "a missing tool was not reported" << std::endl;
        return false;
    }

    const nuocc::ToolResult empty_stdin = nuocc::RunTool(
        {.arguments = {script.string(), "--read-stdin"}, .input_files = {}},
        false);
    if (!empty_stdin.Succeeded())
    {
        std::cerr << "standard input was not redirected from /dev/null"
                  << std::endl;
        return false;
    }

    const pid_t child = fork();
    if (child == -1)
        return false;
    if (child == 0)
    {
        close(STDIN_FILENO);
        const nuocc::ToolResult result = nuocc::RunTool(
            {.arguments = {script.string(), "--read-stdin"}, .input_files = {}},
            true);
        _exit(result.Succeeded() ? EXIT_SUCCESS : EXIT_FAILURE);
    }

    int child_status = 0;
    pid_t waited;
    do
    {
        waited = waitpid(child, &child_status, 0);
    }
    while (waited == -1 && errno == EINTR);
    if (waited == -1 || !WIFEXITED(child_status) ||
        WEXITSTATUS(child_status) != 0)
    {
        std::cerr << "a closed standard input was not replaced" << std::endl;
        return false;
    }

    return true;
}

bool CheckInputFiles(const std::filesystem::path& directory)
{
    const std::filesystem::path first = directory / "first input";
    const std::filesystem::path second = directory / "second input";
    const std::string first_payload(5000, 'a');
    const std::string second_payload(6000, 'b');
    if (!WriteFile(first, first_payload) || !WriteFile(second, second_payload))
        return false;

    const nuocc::ToolResult result = nuocc::RunTool(
        {.arguments = {"cat"}, .input_files = {first.string(), second.string()}},
        true);
    if (!result.Succeeded() || !result.standard_output ||
        *result.standard_output != first_payload + second_payload)
    {
        std::cerr << "input files were not concatenated" << std::endl;
        return false;
    }

    const nuocc::ToolResult no_command =
        nuocc::RunTool({.arguments = {}, .input_files = {}}, false);
    const std::string nul_argument("argument\0suffix", 15);
    const nuocc::ToolResult nul_result = nuocc::RunTool(
        {.arguments = {"/bin/true", nul_argument}, .input_files = {}}, false);
    const nuocc::ToolResult bad_input = nuocc::RunTool(
        {.arguments = {"cat"},
         .input_files = {(directory / "missing input").string()}}, true);
    const std::string nul_path("input\0suffix", 12);
    const nuocc::ToolResult nul_input = nuocc::RunTool(
        {.arguments = {"cat"}, .input_files = {nul_path}}, true);
    if (no_command.error != nuocc::ToolErrorCode::kInvalidCommand ||
        nul_result.error != nuocc::ToolErrorCode::kInvalidCommand ||
        nul_input.error != nuocc::ToolErrorCode::kInput ||
        bad_input.error != nuocc::ToolErrorCode::kInput)
    {
        std::cerr << "invalid commands or inputs were accepted" << std::endl;
        return false;
    }
    return true;
}

}   /* namespace */

int main()
{
    try
    {
        const TemporaryDirectory directory;
        if (!CheckQuote() || !CheckArguments(directory.GetPath()) ||
            !CheckInputFiles(directory.GetPath()))
        {
            return EXIT_FAILURE;
        }
    }
    catch (const std::exception& error)
    {
        std::cerr << "test error: " << error.what() << std::endl;
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
