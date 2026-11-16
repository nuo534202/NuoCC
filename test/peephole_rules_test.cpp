#include <iostream>
#include <string>
#include <vector>

#include "codegen/nuocc_peephole.hpp"
#include "utils/nuocc_error.hpp"

namespace
{

void ReportFailure(const std::string& message)
{
    std::cerr << message << std::endl;
}

bool RejectsRule(const nuocc::PeepholeRule& rule)
{
    try
    {
        const nuocc::Peephole peephole(std::vector<nuocc::PeepholeRule>{rule});
        (void) peephole;
    }
    catch (const nuocc::CompilerError&)
    {
        return true;
    }

    return false;
}

}   /* namespace */

int main()
{
    /*
     * The temporary vector is destroyed before Optimise() runs. The pass
     * must own its rules rather than keep a reference to it.
     */
    const nuocc::Peephole peephole(std::vector<nuocc::PeepholeRule>{
        {.matched = {"first", "second"}, .kept = {"first"}},
    });
    const nuocc::Peephole::Lines code = {
        "\tbefore", "\tfirst", "\tsecond", "\tafter"};
    const nuocc::Peephole::Lines expected = {"\tbefore", "\tfirst", "\tafter"};

    if (peephole.Optimise(code) != expected)
    {
        ReportFailure("the peephole pass did not keep a copy of its rules");
        return 1;
    }

    const std::vector<nuocc::PeepholeRule> invalid = {
        {.matched = {"first"}, .kept = {"first"}},
        {.matched = {"first"}, .kept = {"first", "second"}},
    };

    for (const nuocc::PeepholeRule& rule : invalid)
    {
        if (!RejectsRule(rule))
        {
            ReportFailure("a non-shrinking peephole rule was accepted");
            return 1;
        }
    }

    return 0;
}
