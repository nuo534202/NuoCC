#pragma once

#include <chrono>
#include <cstddef>
#include <optional>
#include <string>
#include <vector>

namespace nuocc
{

/* An external command whose arguments are passed directly to its executable. */
struct ToolCommand
{
    std::vector<std::string> arguments;
    std::vector<std::string> input_files;
};

enum class ToolErrorCode
{
    kNone,
    kInvalidCommand,
    kInput,
    kStart,
    kRead,
    kOutputLimit,
    kWait,
    kTimeout,
    kSignal,
};

constexpr std::size_t kDefaultMaximumOutput = 64 * 1024 * 1024;
constexpr std::chrono::milliseconds kDefaultTimeout{30 * 60 * 1000};

struct ToolResult
{
    std::optional<std::string> standard_output;
    std::optional<std::string> standard_error;
    std::string error_message;
    int exit_status = -1;
    bool exited = false;
    ToolErrorCode error = ToolErrorCode::kNone;

    bool Succeeded() const
    {
        return error == ToolErrorCode::kNone && exited && exit_status == 0;
    }
};

/*
 * Run one tool without a shell and wait for it. Input files, when present,
 * are concatenated and supplied on standard input; otherwise stdin is
 * `/dev/null`. With capture_output enabled, standard_output and standard_error
 * are present once the tool starts, including when it later fails, unless
 * capture itself fails. Their combined size is bounded by maximum_output so
 * that a runaway tool cannot exhaust memory. A non-positive timeout expires
 * before waiting; otherwise the tool is stopped after the timeout. The result
 * also includes the exit status or the failure which prevented a normal exit.
 * Descriptors opened by this module are close-on-exec; caller-owned descriptors
 * should also be close-on-exec before invoking it.
 */
ToolResult RunTool(const ToolCommand& command, bool capture_output,
                   std::size_t maximum_output = kDefaultMaximumOutput,
                   std::chrono::milliseconds timeout = kDefaultTimeout);

/* Quote one POSIX shell word for a diagnostic. This does not execute it. */
std::string Quote(const std::string& text);

/* Render a complete command for a diagnostic. This does not execute it. */
std::string FormatCommand(const ToolCommand& command);

}   /* namespace nuocc */
