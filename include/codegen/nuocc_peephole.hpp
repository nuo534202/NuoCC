#pragma once

#include <string>
#include <vector>

namespace nuocc
{

/*
 * One rule of the peephole optimiser: the lines the code generator leaves
 * one after another, and the lines which are left in the place of them.
 *
 * A line is written the way the code generator writes one, as the name of
 * an instruction and the operand it works on, or as a label. A part of it
 * written %1, %2 and so on stands for any text which is not nothing, and
 * where a rule uses the same part twice the two have to be the same text,
 * so a rule written for `std %1` and then `ldd %1` is about a value which
 * is stored and read back in one place rather than in two.
 *
 * Every rule leaves fewer lines than it matched, so the code of a program
 * only gets shorter while the pass is being made.
 */
struct PeepholeRule
{
    std::vector<std::string> matched;
    std::vector<std::string> kept;
};

/*
 * The peephole optimiser: the pass over the code a target wrote which looks
 * at the few instructions standing at one place, and leaves a shorter
 * sequence there when a rule says the two of them do the same thing.
 *
 * The tutorial gives this pass to the 6809 as a program of its own, cpeep,
 * which reads a file of rules and writes out the code those rules left.
 * Here it is a pass of the compiler over the lines the code generator
 * wrote, and the rules of it are written into the compiler. A rule is a
 * sequence of instructions of one machine and is only right about the code
 * that machine's backend writes, so each machine which has the pass names
 * its own rules and the two travel together.
 */
class Peephole
{
public:
    /* The lines of a program, as the code generator wrote them. */
    using Lines = std::vector<std::string>;

    explicit Peephole(const std::vector<PeepholeRule>& rules);

    /*
     * Put the lines of a program through the rules until none of them
     * fires, and answer with the lines which are left.
     */
    Lines Optimise(const Lines& code) const;

    /*
     * Read the code a target wrote out of the file it was written into, put
     * it through the pass, and write back what the pass left. This is what
     * a target's OptimiseCode() asks for.
     */
    bool OptimiseFile(const std::string& file) const;

private:
    const std::vector<PeepholeRule>& rules_;
};

/*
 * The rules of each machine which has the pass. A machine with no rules
 * here has no pass: the QBE target writes QBE's intermediate language
 * rather than the code of a machine, and passes no rules of its own.
 */
const std::vector<PeepholeRule>& Arm64PeepholeRules();
const std::vector<PeepholeRule>& M6809PeepholeRules();
const std::vector<PeepholeRule>& X86PeepholeRules();

}   /* namespace nuocc */
