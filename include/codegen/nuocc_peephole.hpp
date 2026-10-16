#pragma once

#include <string>
#include <vector>

namespace nuocc
{

/*
 * The peephole optimiser: the pass over the code a target wrote which looks
 * at the few instructions standing at one place, and leaves a shorter
 * sequence there when a rule says the two of them do the same thing.
 *
 * The tutorial gives this pass to the 6809 as a program of its own, cpeep,
 * which reads a file of rules and writes out the code those rules left.
 * Here it is a pass of the compiler over the lines the code generator
 * wrote, and the rules of it are written into the compiler: the ones the
 * code the 6809 backend writes needs.
 *
 * A rule is a sequence of lines the code generator leaves one after
 * another, together with the lines which are left in their place. A line of
 * a rule is written the way the code generator writes one, as the name of
 * an instruction and the operand it works on, or as a label. A part of it
 * written %1, %2 and so on stands for any text which is not nothing, and
 * where a rule uses the same part twice the two have to be the same text,
 * so a rule written for `std %1` and then `ldd %1` is about a value which
 * is stored and read back in one place rather than in two.
 */
class Peephole
{
public:
    /* The lines of a program, as the code generator wrote them. */
    using Lines = std::vector<std::string>;

    /*
     * Put the lines of a program through the rules until none of them
     * fires, and answer with the lines which are left.
     */
    Lines Optimise(const Lines& code) const;
};

}   /* namespace nuocc */
