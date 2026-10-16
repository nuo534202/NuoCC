#include "codegen/nuocc_peephole.hpp"

#include <array>
#include <cctype>
#include <cstddef>
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
 * One rule of the pass: the lines it looks for, standing one after another,
 * and the lines which are left in the place of them. Every one of them
 * leaves fewer lines than it matched, so the code of a program only gets
 * shorter while the pass is being made.
 */
struct Rule
{
    std::vector<CodeLine> matched;
    std::vector<CodeLine> kept;
};

/* A line of a rule which is an instruction, and what it works on. */
CodeLine Instruction(std::string name, std::string operand = "")
{
    CodeLine line;
    line.name = std::move(name);
    line.operand = std::move(operand);

    return line;
}

/* A line of a rule which is a label, written as the whole of the label. */
CodeLine Label(std::string shape)
{
    CodeLine line;
    line.is_label = true;
    line.name = std::move(shape);

    return line;
}

/*
 * The rules of the pass.
 *
 * Each of them takes away an instruction which the machine does not need,
 * and each of them is about one place twice where it has to be, so that
 * what is taken away cannot be about two different places.
 */
const std::vector<Rule> kRules = {
    /*
     * A value which is stored and then read straight back is the value the
     * machine holds in D still: a store from D leaves the value there, so
     * the read brings back what the register holds already. The code
     * generator writes this pair where the value one expression was worked
     * out into has been put in its place and the next expression begins by
     * asking for it.
     */
    {{Instruction("std", "%1"), Instruction("ldd", "%1")},
     {Instruction("std", "%1")}},
    /* The same for the address the machine holds in X, which a store from
     * X leaves there. */
    {{Instruction("stx", "%1"), Instruction("ldx", "%1")},
     {Instruction("stx", "%1")}},
    /*
     * A jump to the label standing next to it is taken away: the machine
     * comes to the same instruction by carrying on to it.
     */
    {{Instruction("jmp", "%1"), Label("%1:")},
     {Label("%1:")}},
};

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

Peephole::Lines Peephole::Optimise(const Lines& code) const
{
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

            for (const Rule& candidate : kRules)
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

}   /* namespace nuocc */
