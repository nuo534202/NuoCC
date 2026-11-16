#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <string>
#include <system_error>
#include <vector>

#include "codegen/nuocc_peephole.hpp"

namespace
{

std::string ReadFile(const std::filesystem::path& path)
{
    std::ifstream input(path, std::ios::in | std::ios::binary);
    return std::string(std::istreambuf_iterator<char>(input),
                       std::istreambuf_iterator<char>());
}

void ReportFailure(const std::string& message)
{
    std::cerr << message << std::endl;
}

}   /* namespace */

int main()
{
    namespace fs = std::filesystem;

    std::error_code error;
    const auto stamp = std::chrono::steady_clock::now().time_since_epoch().count();
    const fs::path directory =
        fs::temp_directory_path(error) /
        ("nuocc-peephole-file-test-" + std::to_string(stamp));

    if (error || !fs::create_directories(directory, error) || error)
    {
        ReportFailure("could not create the test directory");
        return 1;
    }

    const fs::path missing = directory / "missing.s";
    const fs::path code = directory / "program.s";
    const std::string original =
        "before\n\tldd\t#1\n\tstd\t2,u\nafter\n";
    const std::vector<nuocc::PeepholeRule> rules = {
        {.matched = {"\tldd\t#1", "\tstd\t2,u"},
         .kept = {"\tldd\t#1"}},
    };
    const nuocc::Peephole peephole(rules);

    {
        std::ofstream output(code, std::ios::out | std::ios::binary |
                                       std::ios::trunc);
        output << original;

        if (!output)
        {
            ReportFailure("could not write the original test file");
            return 1;
        }
    }

    if (peephole.OptimiseFile(missing.string()))
    {
        ReportFailure("a missing source file was accepted");
        return 1;
    }

    const fs::perms original_permissions = fs::status(directory).permissions();
    fs::permissions(directory, fs::perms::owner_read | fs::perms::owner_exec,
                    fs::perm_options::replace, error);

    if (error)
    {
        ReportFailure("could not make the test directory read-only");
        return 1;
    }

    const bool optimised = peephole.OptimiseFile(code.string());

    fs::permissions(directory, original_permissions, fs::perm_options::replace,
                    error);

    if (error)
    {
        ReportFailure("could not restore the test directory permissions");
        return 1;
    }

    if (optimised)
    {
        ReportFailure("an unwritable output directory was accepted");
        return 1;
    }

    if (ReadFile(code) != original)
    {
        ReportFailure("a failed optimisation changed the original file");
        return 1;
    }

    fs::path temporary = code;
    temporary += ".tmp";

    if (fs::exists(temporary))
    {
        ReportFailure("a failed optimisation left its temporary file");
        return 1;
    }

    fs::remove_all(directory, error);
    return 0;
}
