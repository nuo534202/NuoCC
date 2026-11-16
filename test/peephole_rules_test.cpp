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

    const std::vector<nuocc::PeepholeRule> invalid_placeholders = {
        {.matched = {"first %0", "second"}, .kept = {"first"}},
        {.matched = {"first %10", "second"}, .kept = {"first"}},
        {.matched = {"first %1", "second"}, .kept = {"first %2"}},
    };

    for (const nuocc::PeepholeRule& rule : invalid_placeholders)
    {
        if (!RejectsRule(rule))
        {
            ReportFailure("an invalid peephole placeholder was accepted");
            return 1;
        }
    }

    const nuocc::Peephole m6809(nuocc::M6809PeepholeRules());
    const nuocc::Peephole::Lines flag_setters = {
        "\tstd\t2,u",
        "\tldd\t2,u",
        "\tlbeq\tdone",
        "\tstx\t6,u",
        "\tldx\t6,u",
        "\tlbeq\tdone",
    };

    if (m6809.Optimise(flag_setters) != flag_setters)
    {
        ReportFailure("a 6809 flag-setting load was removed");
        return 1;
    }

    const nuocc::Peephole x86(nuocc::X86PeepholeRules());
    const nuocc::Peephole::Lines x86_moves = {
        "\tmovq\t%rax, 8(%rbp)",
        "\tmovq\t8(%rbp), %rax",
        "\tmovq\t8(%rbp), %rax",
        "\tmovq\t%rax, 8(%rbp)",
    };

    if (x86.Optimise(x86_moves) != x86_moves)
    {
        ReportFailure("an x86 load or store was removed without role checks");
        return 1;
    }

    return 0;
}
