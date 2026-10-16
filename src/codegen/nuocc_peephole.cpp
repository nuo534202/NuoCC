#include "codegen/nuocc_peephole.hpp"

#include <array>
#include <cctype>
#include <cstddef>
#include <fstream>
#include <iostream>
#include <utility>

namespace nuocc
{

namespace
{

/*
 * A line of the code, cut into the parts a rule is written in: the name of
 * an instruction and the operand it works on, or the label the line gives.
 * A line which is neither -- a directive, or a line of nothing -- is left
 * with the text it was written as and nothing else, and no rule looks at
 * it, since every rule says what instructions it is written for.
 */
struct CodeLine
{
    /* The line as the code generator wrote it. */
    std::string text;
    /* The name of the instruction, or the whole of a label. */
    std::string name;
    /* What the instruction works on, if it works on anything. */
    std::string operand;
    /* Whether the line is a label of its own rather than an instruction. */
    bool is_label = false;
};

/*
 * What the parts of a rule hold while it is being matched. The parts are
 * written %1 to %9, so the place of a part in this is the digit which names
 * it. A part which holds nothing is one the rule has not come to yet, since
 * a part never stands for nothing.
 */
using Parts = std::array<std::string, 10>;

/*
 * Read a line, of the code or of a rule, as what it holds. Both are written
 * the same way, so the rules are read by the same reading.
 */
CodeLine ParseLine(const std::string& text)
{
    CodeLine line;
    line.text = text;

    const std::size_t first = text.find_first_not_of(" \t");

    if (first == std::string::npos)
        return line;

    const std::string body =
        text.substr(first, text.find_last_not_of(" \t") - first + 1);

    /*
     * A label stands on a line of its own, and the colon at the end of it
     * says what it is.
     */
    if (body.back() == ':')
    {
        line.is_label = true;
        line.name = body;
        return line;
    }

    const std::size_t split = body.find_first_of(" \t");
    line.name = body.substr(0, split);

    if (split == std::string::npos)
        return line;

    const std::size_t operand = body.find_first_not_of(" \t", split);

    if (operand != std::string::npos)
        line.operand = body.substr(operand);

    return line;
}

/*
 * Whether some text is what the shape of a line says it is, and if it is,
 * what the parts of the shape hold. A part stands for the text up to what
 * the shape says next, or for the rest of the text where the part ends the
 * shape; it never stands for nothing, and where a shape uses the same part
 * twice the two have to be the same text.
 */
bool MatchesShape(const std::string& shape,
    const std::string& text,
    Parts& parts)
{
    std::size_t at = 0;

    for (std::size_t k = 0; k < shape.size();)
    {
        const bool is_part = shape[k] == '%' && k + 1 < shape.size() &&
            std::isdigit(static_cast<unsigned char>(shape[k + 1]));

        if (!is_part)
        {
            if (at == text.size() || text[at] != shape[k])
                return false;

            at++;
            k++;
            continue;
        }

        const std::size_t part = static_cast<std::size_t>(shape[k + 1] - '0');
        std::string held;

        /*
         * What a part stands for runs up to the character of the shape
         * which follows it, which is where the shape says the part ends. A
         * part which has nothing after it in the shape is the rest of the
         * text, and one followed by another part is not something the
         * rules are written with.
         */
        if (k + 2 == shape.size() || shape[k + 2] == '%')
            held = text.substr(at);
        else
        {
            const std::size_t ends = text.find(shape[k + 2], at);

            if (ends == std::string::npos)
                return false;

            held = text.substr(at, ends - at);
        }

        if (held.empty())
            return false;

        if (!parts[part].empty() && parts[part] != held)
            return false;

        parts[part] = std::move(held);
        at += parts[part].size();
        k += 2;
    }

    return at == text.size();
}

/*
 * Whether the line of the code is what the line of a rule is written for,
 * and if it is, what the parts of the rule hold. A line of a rule which is
 * a label is about a label; any other is about an instruction of the name
 * it gives, working on the operand its shape says.
 */
bool Matches(const CodeLine& wanted, const CodeLine& line, Parts& parts)
{
    if (wanted.is_label != line.is_label)
        return false;

    if (wanted.is_label)
        return MatchesShape(wanted.name, line.name, parts);

    return wanted.name == line.name &&
           MatchesShape(wanted.operand, line.operand, parts);
}

/*
 * The rules of the 6809.
 *
 * Each of them takes away an instruction which the machine does not need,
 * and each of them is about one place twice where it has to be, so that
 * what is taken away cannot be about two different places.
 */
const std::vector<PeepholeRule> kM6809Rules = {
    /*
     * A value which is stored and then read straight back is the value the
     * machine holds in D still: a store from D leaves the value there, so
     * the read brings back what the register holds already. The code
     * generator writes this pair where the value one expression was worked
     * out into has been put in its place and the next expression begins by
     * asking for it.
     */
    {{"std %1", "ldd %1"}, {"std %1"}},
    /* The same for the address the machine holds in X, which a store from
     * X leaves there. */
    {{"stx %1", "ldx %1"}, {"stx %1"}},
    /*
     * A jump to the label standing next to it is taken away: the machine
     * comes to the same instruction by carrying on to it.
     */
    {{"jmp %1", "%1:"}, {"%1:"}},
};

/*
 * The rules of the arm64.
 *
 * A read may be taken away after a store of the same width to the same
 * place, because a register keeps the value a store from it leaves and the
 * read of a value that wide brings back exactly what the register holds.
 * A read of a narrower value is left alone, since it sign extends or zero
 * extends what it reads and the register may hold the wide value the
 * program worked out rather than the narrow one which is stored.
 */
const std::vector<PeepholeRule> kArm64Rules = {
    /*
     * A value of eight bytes, a long or an address, which is stored and
     * then read straight back is the value the register holds still, so
     * the read is left out. The code generator writes this pair where the
     * value one expression was worked out into has been put in its place
     * and the next expression begins by asking for it.
     */
    {{"str %1, %2", "ldr %1, %2"}, {"str %1, %2"}},
    /*
     * The same for a char, which is a value of one byte and has no sign of
     * its own: a register holding one holds the whole of it, so the read
     * of it brings back what the register holds already.
     */
    {{"strb %1, %2", "ldrb %1, %2"}, {"strb %1, %2"}},
    /*
     * A branch to the label standing next to it is taken away: the machine
     * comes to the same instruction by carrying on to it. What a return
     * statement leaves is this.
     */
    {{"b %1", "%1:"}, {"%1:"}},
};

/*
 * The rules of the x86-64, which are the instructions of this machine
 * rather than of the arm64 and are about the same values for the same
 * reasons: what is left of a value of eight bytes travels whole, so a long
 * or an address which is stored and then read straight back is still in
 * the register the store came from, and a jump to the label standing next
 * to it is taken away.
 *
 * A read of a narrower value is left alone, as it is on the arm64: the
 * read of a value of four bytes sign extends it, and the register holds
 * the value the program worked out, which may be wider than the four bytes
 * the machine stores. The read of a char is left alone as well, though a
 * char has no sign of its own, because the two instructions name the
 * register differently -- a store of one byte writes %r8b where a read of
 * it writes %r8 -- and a rule can only say that the same text stands on
 * two lines.
 */
const std::vector<PeepholeRule> kX86Rules = {
    {{"movq %1, %2", "movq %2, %1"}, {"movq %1, %2"}},
    {{"jmp %1", "%1:"}, {"%1:"}},
};

/*
 * One rule of the pass, with the lines of it read as what they hold. The
 * text a machine writes its rules in is read once, and what the pass
 * matches the code against is this.
 */
struct Rule
{
    std::vector<CodeLine> matched;
    std::vector<CodeLine> kept;
};

/* Read the rules of a machine, so that the pass may match the code. */
std::vector<Rule> ReadRules(const std::vector<PeepholeRule>& rules)
{
    std::vector<Rule> read;

    for (const PeepholeRule& rule : rules)
    {
        Rule one;

        for (const std::string& line : rule.matched)
            one.matched.push_back(ParseLine(line));

        for (const std::string& line : rule.kept)
            one.kept.push_back(ParseLine(line));

        read.push_back(std::move(one));
    }

    return read;
}

/*
 * Whether the lines of the code standing at a place are what a rule looks
 * for, and if they are, what the parts of the rule hold.
 */
bool Matches(const Rule& rule,
    const std::vector<CodeLine>& code,
    std::size_t at,
    Parts& parts)
{
    if (rule.matched.size() > code.size() - at)
        return false;

    for (std::size_t k = 0; k < rule.matched.size(); k++)
        if (!Matches(rule.matched[k], code[at + k], parts))
            return false;

    return true;
}

/* The text of a shape, with every part of it written out. */
std::string Substituted(const std::string& shape, const Parts& parts)
{
    std::string text;

    for (std::size_t k = 0; k < shape.size();)
    {
        const bool is_part = shape[k] == '%' && k + 1 < shape.size() &&
            std::isdigit(static_cast<unsigned char>(shape[k + 1]));

        if (!is_part)
        {
            text += shape[k];
            k++;
            continue;
        }

        text += parts[static_cast<std::size_t>(shape[k + 1] - '0')];
        k += 2;
    }

    return text;
}

/*
 * The line of code a rule leaves, written the way the code generator writes
 * one, with every part of it standing for the text that part held.
 */
std::string Written(const CodeLine& line, const Parts& parts)
{
    if (line.is_label)
        return Substituted(line.name, parts);

    if (line.operand.empty())
        return "\t" + line.name;

    return "\t" + line.name + "\t" + Substituted(line.operand, parts);
}

}   /* namespace */

Peephole::Peephole(const std::vector<PeepholeRule>& rules) : rules_(rules) {}

Peephole::Lines Peephole::Optimise(const Lines& code) const
{
    const std::vector<Rule> rules = ReadRules(rules_);

    Lines left = code;
    bool fired = true;

    /*
     * Every rule leaves fewer lines than it matched, so the code only gets
     * shorter and this comes to an end. A rule which fires may leave the
     * lines around it in a shape another rule is written for, so the pass
     * is made again until it changes nothing.
     */
    while (fired)
    {
        fired = false;

        std::vector<CodeLine> lines;

        for (const std::string& text : left)
            lines.push_back(ParseLine(text));

        std::vector<std::string> written;
        std::size_t at = 0;

        while (at < lines.size())
        {
            const Rule *rule = nullptr;
            Parts parts;

            for (const Rule& candidate : rules)
            {
                Parts held;

                if (Matches(candidate, lines, at, held))
                {
                    rule = &candidate;
                    parts = std::move(held);
                    break;
                }
            }

            /* Nothing is written for this line, so it is left as it is. */
            if (rule == nullptr)
            {
                written.push_back(lines[at].text);
                at++;
                continue;
            }

            for (const CodeLine& line : rule->kept)
                written.push_back(Written(line, parts));

            at += rule->matched.size();
            fired = true;
        }

        left = std::move(written);
    }

    return left;
}

bool Peephole::OptimiseFile(const std::string& file) const
{
    std::ifstream source(file);

    if (!source.is_open())
    {
        std::cerr << "error: cannot read the code in " << file << std::endl;
        return false;
    }

    Lines lines;

    for (std::string line; std::getline(source, line);)
        lines.push_back(line);

    source.close();

    const Lines left = Optimise(lines);

    std::ofstream written(file, std::ios::out | std::ios::trunc);

    if (!written.is_open())
    {
        std::cerr << "error: cannot write the code to " << file << std::endl;
        return false;
    }

    for (const std::string& line : left)
        written << line << '\n';

    if (!written)
    {
        std::cerr << "error: cannot write the code to " << file << std::endl;
        return false;
    }

    return true;
}

const std::vector<PeepholeRule>& Arm64PeepholeRules()
{
    return kArm64Rules;
}

const std::vector<PeepholeRule>& M6809PeepholeRules()
{
    return kM6809Rules;
}

const std::vector<PeepholeRule>& X86PeepholeRules()
{
    return kX86Rules;
}

}   /* namespace nuocc */
