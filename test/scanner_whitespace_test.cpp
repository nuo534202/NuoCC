#include <chrono>
#include <condition_variable>
#include <cstddef>
#include <cstdlib>
#include <iostream>
#include <mutex>
#include <stop_token>
#include <string>
#include <string_view>
#include <thread>
#include <utility>
#include <vector>

#include "nuocc_scanner.hpp"
#include "utils/nuocc_error.hpp"

namespace
{

constexpr std::chrono::seconds kWatchdogTimeout(5);

class Watchdog
{
public:
    explicit Watchdog(std::chrono::seconds timeout) :
        thread_([this, timeout](std::stop_token stop_token)
        {
            Wait(stop_token, timeout);
        })
    {
    }

    ~Watchdog() = default;

    void SetCase(std::string_view name)
    {
        std::lock_guard lock(mutex_);
        active_case_ = name;
    }

    Watchdog(const Watchdog&) = delete;
    Watchdog& operator=(const Watchdog&) = delete;

private:
    void Wait(std::stop_token stop_token, std::chrono::seconds timeout)
    {
        std::unique_lock lock(mutex_);
        condition_.wait_for(lock, stop_token, timeout, []
        {
            return false;
        });

        if (stop_token.stop_requested())
            return;

        std::cerr << "scanner test timed out";
        if (!active_case_.empty())
            std::cerr << " in case '" << active_case_ << "'";
        std::cerr << std::endl;
        std::_Exit(EXIT_FAILURE);
    }

    std::mutex mutex_;
    std::condition_variable_any condition_;
    std::string active_case_;

    /* The thread is declared last so it joins before the state it uses. */
    std::jthread thread_;
};

bool CheckIdentifier(const nuocc::NodePtr& token, std::string_view name)
{
    const auto* identifier =
        dynamic_cast<const nuocc::Identifier*>(token.get());

    return identifier != nullptr && identifier->GetName() == name;
}

bool CheckTwoIdentifiers(const std::string& text)
{
    nuocc::Scanner scanner;
    scanner.Scan(text);

    const std::vector<nuocc::NodePtr>& tokens = scanner.GetTokenList();

    if (tokens.size() != 3)
    {
        std::cerr << "test input produced " << tokens.size()
                  << " tokens instead of 3" << std::endl;
        return false;
    }

    if (!CheckIdentifier(tokens[0], "first"))
    {
        std::cerr << "token 0 is not the identifier first" << std::endl;
        return false;
    }

    if (!CheckIdentifier(tokens[1], "second"))
    {
        std::cerr << "token 1 is not the identifier second" << std::endl;
        return false;
    }

    if (tokens[2]->GetNodeTag() != nuocc::T_EOF)
    {
        std::cerr << "token 2 is not the end of the token list" << std::endl;
        return false;
    }

    return true;
}

bool CheckWhitespace(const std::string& whitespace)
{
    return CheckTwoIdentifiers(
        "first" + whitespace + "second" + whitespace);
}

bool CheckDirectiveAfterLineBreak(std::string_view line_break)
{
    const std::string break_text(line_break);
    const std::string text = "first" + break_text + "# 2 \"file.c\"" +
        break_text + "second" + break_text;

    return CheckTwoIdentifiers(text);
}

bool CheckLeadingDirective(std::string_view line_break)
{
    const std::string break_text(line_break);
    const std::string text = "# 2 \"file.c\"" + break_text + "first" +
        break_text + "second" + break_text;

    return CheckTwoIdentifiers(text);
}

bool CheckEndOnly(const std::string& text)
{
    nuocc::Scanner scanner;
    scanner.Scan(text);

    const std::vector<nuocc::NodePtr>& tokens = scanner.GetTokenList();

    if (tokens.size() != 1 || tokens[0]->GetNodeTag() != nuocc::T_EOF)
    {
        std::cerr << "end-only case produced unexpected tokens" << std::endl;
        return false;
    }

    return true;
}

bool CheckWhitespaceOnly(const std::string& whitespace)
{
    return CheckEndOnly(whitespace);
}

bool CheckStringLiteral(const std::string& expected, const std::string& text)
{
    nuocc::Scanner scanner;
    scanner.Scan(text);

    const std::vector<nuocc::NodePtr>& tokens = scanner.GetTokenList();
    if (tokens.size() != 2 || tokens[0]->GetNodeTag() != nuocc::T_StrLit ||
        tokens[1]->GetNodeTag() != nuocc::T_EOF)
    {
        std::cerr << "string literal case produced unexpected tokens"
                  << std::endl;
        return false;
    }

    const auto* literal = dynamic_cast<
        const nuocc::Literal<std::string, nuocc::T_StrLit>*>(tokens[0].get());
    if (literal == nullptr || literal->GetValue() != expected)
    {
        std::cerr << "string literal case produced a different value"
                  << std::endl;
        return false;
    }

    return true;
}

bool CheckCharacterLiteral(int expected, const std::string& text)
{
    nuocc::Scanner scanner;
    scanner.Scan(text);

    const std::vector<nuocc::NodePtr>& tokens = scanner.GetTokenList();
    if (tokens.size() != 2 || tokens[0]->GetNodeTag() != nuocc::T_IntLit ||
        tokens[1]->GetNodeTag() != nuocc::T_EOF)
    {
        std::cerr << "character literal case produced unexpected tokens"
                  << std::endl;
        return false;
    }

    const auto* literal = dynamic_cast<
        const nuocc::Literal<int, nuocc::T_IntLit>*>(tokens[0].get());
    if (literal == nullptr || literal->GetValue() != expected)
    {
        std::cerr << "character literal case produced a different value"
                  << std::endl;
        return false;
    }

    return true;
}

bool CheckScannerReuse()
{
    nuocc::Scanner scanner;
    for (const std::string name : {"first", "first", "second", ""})
    {
        scanner.Scan(name);
        const auto& tokens = scanner.GetTokenList();
        if (tokens.size() != (name.empty() ? 1 : 2) ||
            tokens.back()->GetNodeTag() != nuocc::T_EOF ||
            (!name.empty() && !CheckIdentifier(tokens[0], name)))
        {
            std::cerr << "repeated scan retained earlier tokens" << std::endl;
            return false;
        }
    }

    try
    {
        scanner.Scan("partial @");
        std::cerr << "scanner accepted invalid input" << std::endl;
        return false;
    }
    catch (const nuocc::CompilerError& error)
    {
        if (std::string_view(error.what()) !=
                "Error: unrecognized character @!" ||
            !scanner.GetTokenList().empty())
        {
            std::cerr << "failed scan left an invalid token list or error: "
                      << error.what() << std::endl;
            return false;
        }
    }

    scanner.Scan("recovered");
    const auto& tokens = scanner.GetTokenList();
    return tokens.size() == 2 && CheckIdentifier(tokens[0], "recovered") &&
        tokens[1]->GetNodeTag() == nuocc::T_EOF;
}

bool CheckDotOperators()
{
    struct DotCase
    {
        std::string text;
        std::vector<nuocc::NodeTag> tags;
    };

    const DotCase cases[] = {
        {"a..x", {nuocc::T_Identifier, nuocc::T_Dot, nuocc::T_Dot,
                  nuocc::T_Identifier, nuocc::T_EOF}},
        {"..", {nuocc::T_Dot, nuocc::T_Dot, nuocc::T_EOF}},
        {"...", {nuocc::T_Ellipsis, nuocc::T_EOF}},
    };

    for (const DotCase& test_case : cases)
    {
        nuocc::Scanner scanner;
        scanner.Scan(test_case.text);
        const auto& tokens = scanner.GetTokenList();
        if (tokens.size() != test_case.tags.size())
        {
            std::cerr << "dot case produced the wrong token count: "
                      << test_case.text << std::endl;
            return false;
        }
        for (std::size_t index = 0; index < tokens.size(); ++index)
        {
            if (tokens[index]->GetNodeTag() != test_case.tags[index])
            {
                std::cerr << "dot case " << test_case.text
                          << " produced the wrong tag at token " << index
                          << std::endl;
                return false;
            }
        }
    }
    return true;
}

bool CheckEscapedByte(int expected, const std::string& escape)
{
    if (!CheckCharacterLiteral(expected, "'" + escape + "'"))
        return false;

    nuocc::Scanner scanner;
    scanner.Scan("\"" + escape + "z\"");
    const auto& tokens = scanner.GetTokenList();
    if (tokens.size() != 2 || tokens[1]->GetNodeTag() != nuocc::T_EOF)
        return false;

    const auto* literal = dynamic_cast<
        const nuocc::Literal<std::string, nuocc::T_StrLit>*>(tokens[0].get());
    return literal != nullptr && literal->GetValue().size() == 2 &&
        static_cast<unsigned char>(literal->GetValue()[0]) == expected &&
        literal->GetValue()[1] == 'z';
}

bool CheckEscapedHighBytes()
{
    constexpr char kHexDigits[] = "0123456789abcdef";
    for (int value = 0x80; value <= 0xff; ++value)
    {
        const std::string hexadecimal = std::string("\\x") +
            kHexDigits[value >> 4] + kHexDigits[value & 15];
        const std::string octal = std::string("\\") +
            static_cast<char>('0' + (value >> 6)) +
            static_cast<char>('0' + ((value >> 3) & 7)) +
            static_cast<char>('0' + (value & 7));
        const std::pair<std::string_view, std::string> escapes[] = {
            {"hexadecimal", hexadecimal},
            {"octal", octal},
        };
        for (const auto& [radix, escape] : escapes)
        {
            if (!CheckEscapedByte(value, escape))
            {
                std::cerr << radix << " escaped byte "
                          << std::string(escape) << " failed" << std::endl;
                return false;
            }
        }
    }

    const char raw = static_cast<char>(0xe9);
    if (!CheckCharacterLiteral(0xe9, std::string("'") + raw + "'"))
        return false;

    nuocc::Scanner scanner;
    scanner.Scan(std::string("\"") + raw + "\"");
    const auto& tokens = scanner.GetTokenList();
    if (tokens.size() != 2 || tokens[1]->GetNodeTag() != nuocc::T_EOF)
        return false;
    const auto* literal = dynamic_cast<
        const nuocc::Literal<std::string, nuocc::T_StrLit>*>(tokens[0].get());
    return literal != nullptr && literal->GetValue().size() == 1 &&
        static_cast<unsigned char>(literal->GetValue().front()) == 0xe9;
}

bool CheckDirectiveAtEnd(std::string_view line_break)
{
    return CheckEndOnly("# 2 \"file.c\"" + std::string(line_break));
}

int RunFailureCase(std::string_view argument)
{
    nuocc::Scanner scanner;

    if (argument == "--unterminated-character-cr")
    {
        scanner.Scan("'\r");
    }
    else if (argument == "--unterminated-character-eof")
    {
        scanner.Scan("'");
    }
    else if (argument == "--unterminated-string-cr")
    {
        scanner.Scan("\"abc\rnext\"");
    }
    else if (argument == "--unterminated-string-eof")
    {
        scanner.Scan("\"abc");
    }
    else if (argument == "--trailing-escape-crlf")
    {
        scanner.Scan("\"abc\\\r\n");
    }
    else if (argument == "--trailing-escape-eof")
    {
        scanner.Scan(R"("abc\)");
    }
    else if (argument == "--inline-directive")
    {
        scanner.Scan("first # 2 \"file.c\"");
    }
    else
    {
        std::cerr << "unknown scanner test case: " << argument << std::endl;
        return EXIT_FAILURE;
    }

    std::cerr << "scanner accepted invalid input" << std::endl;
    return EXIT_FAILURE;
}

}    /* namespace */

int main(int argc, char* argv[])
{
    Watchdog watchdog(kWatchdogTimeout);

    if (argc > 1)
    {
        watchdog.SetCase(std::string("failure ") + argv[1]);
        try
        {
            return RunFailureCase(argv[1]);
        }
        catch (const nuocc::CompilerError& error)
        {
            std::cerr << error.what() << std::endl;
            return EXIT_FAILURE;
        }
    }

    const std::vector<std::pair<std::string, std::string>> cases = {
        {"CR", "\r"},
        {"LF", "\n"},
        {"CRLF", "\r\n"},
        {"tab and spaces", "\t  \t"},
        {"vertical tab", "\v"},
        {"form feed", "\f"},
        {"mixed", " \t\n\r\v\f"},
    };

    for (const auto& [name, whitespace] : cases)
    {
        watchdog.SetCase("whitespace " + name);
        if (!CheckWhitespace(whitespace))
        {
            std::cerr << "case: " << name << std::endl;
            return EXIT_FAILURE;
        }

        watchdog.SetCase("whitespace-only " + name);
        if (!CheckWhitespaceOnly(whitespace))
        {
            std::cerr << "whitespace-only case: " << name << std::endl;
            return EXIT_FAILURE;
        }
    }

    const std::vector<std::pair<std::string, std::string>> line_breaks = {
        {"LF", "\n"},
        {"CR", "\r"},
        {"CRLF", "\r\n"},
    };

    for (const auto& [name, line_break] : line_breaks)
    {
        watchdog.SetCase("directive after " + name);
        if (!CheckDirectiveAfterLineBreak(line_break))
        {
            std::cerr << "directive case: " << name << std::endl;
            return EXIT_FAILURE;
        }

        watchdog.SetCase("leading directive " + name);
        if (!CheckLeadingDirective(line_break))
        {
            std::cerr << "leading directive case: " << name << std::endl;
            return EXIT_FAILURE;
        }

        watchdog.SetCase("final directive " + name);
        if (!CheckDirectiveAtEnd(line_break))
        {
            std::cerr << "final directive case: " << name << std::endl;
            return EXIT_FAILURE;
        }
    }

    watchdog.SetCase("final directive at end of input");
    if (!CheckEndOnly("# 2 \"file.c\""))
    {
        std::cerr << "directive at end of input failed" << std::endl;
        return EXIT_FAILURE;
    }

    const std::string literal_whitespace = " \t\v\f";
    const std::string string_value = "a" + literal_whitespace + "b";

    watchdog.SetCase("whitespace inside a string literal");
    if (!CheckStringLiteral(string_value, "\"" + string_value + "\""))
    {
        std::cerr << "string literal whitespace case failed" << std::endl;
        return EXIT_FAILURE;
    }

    watchdog.SetCase("space character literal");
    if (!CheckCharacterLiteral(' ', "' '"))
    {
        std::cerr << "character literal whitespace case failed" << std::endl;
        return EXIT_FAILURE;
    }

    watchdog.SetCase("scanner reuse after successful and failed scans");
    if (!CheckScannerReuse())
    {
        std::cerr << "scanner reuse case failed" << std::endl;
        return EXIT_FAILURE;
    }

    watchdog.SetCase("complete and split dot operators");
    if (!CheckDotOperators())
        return EXIT_FAILURE;

    watchdog.SetCase("octal and hexadecimal high bytes");
    if (!CheckEscapedHighBytes())
        return EXIT_FAILURE;

    return EXIT_SUCCESS;
}
