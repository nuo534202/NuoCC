#include "utils/nuocc_command.hpp"

#include <algorithm>
#include <cerrno>
#include <csignal>
#include <cstdlib>
#include <cstring>
#include <chrono>
#include <filesystem>
#include <poll.h>
#include <string>
#include <utility>
#include <vector>

#include <fcntl.h>
#include <spawn.h>
#include <sys/wait.h>
#include <unistd.h>

extern char** environ;

namespace nuocc
{

namespace
{

class FileDescriptor
{
public:
    FileDescriptor() = default;
    explicit FileDescriptor(int descriptor) : descriptor_(descriptor) {}

    ~FileDescriptor()
    {
        Reset();
    }

    FileDescriptor(const FileDescriptor&) = delete;
    FileDescriptor& operator=(const FileDescriptor&) = delete;

    FileDescriptor(FileDescriptor&& other) noexcept :
        descriptor_(std::exchange(other.descriptor_, -1))
    {
    }

    FileDescriptor& operator=(FileDescriptor&& other) noexcept
    {
        if (this != &other)
        {
            Reset();
            descriptor_ = std::exchange(other.descriptor_, -1);
        }
        return *this;
    }

    int Get() const
    {
        return descriptor_;
    }

    void Reset(int descriptor = -1)
    {
        if (descriptor_ != -1)
            close(descriptor_);
        descriptor_ = descriptor;
    }

private:
    int descriptor_ = -1;
};

ToolResult Failure(ToolErrorCode code, std::string message)
{
    ToolResult result;
    result.error = code;
    result.error_message = std::move(message);
    return result;
}

ToolResult ClassifyStatus(int status)
{
    if (WIFSIGNALED(status))
    {
        return Failure(ToolErrorCode::kSignal,
            "terminated by signal " + std::to_string(WTERMSIG(status)));
    }
    if (!WIFEXITED(status))
        return Failure(ToolErrorCode::kWait, "did not exit normally");

    ToolResult result;
    result.exit_status = WEXITSTATUS(status);
    result.exited = true;
    return result;
}

bool SetCloseOnExec(int descriptor, std::string& error)
{
    const int flags = fcntl(descriptor, F_GETFD);
    if (flags != -1 && fcntl(descriptor, F_SETFD, flags | FD_CLOEXEC) != -1)
        return true;

    error = std::strerror(errno);
    return false;
}

bool RaiseDescriptor(FileDescriptor& descriptor, std::string& error)
{
    if (descriptor.Get() <= STDERR_FILENO)
    {
#ifdef F_DUPFD_CLOEXEC
        const int raised = fcntl(
            descriptor.Get(), F_DUPFD_CLOEXEC, STDERR_FILENO + 1);
#else
        const int raised = fcntl(descriptor.Get(), F_DUPFD, STDERR_FILENO + 1);
#endif
        if (raised == -1)
        {
            error = std::strerror(errno);
            return false;
        }
        descriptor.Reset(raised);
    }
    return SetCloseOnExec(descriptor.Get(), error);
}

bool CreatePipe(FileDescriptor& read_end, FileDescriptor& write_end,
    std::string& error)
{
    int descriptors[2] = {-1, -1};
#if defined(__linux__)
    const int result = pipe2(descriptors, O_CLOEXEC);
#else
    const int result = pipe(descriptors);
#endif
    if (result != 0)
    {
        error = std::strerror(errno);
        return false;
    }

    read_end.Reset(descriptors[0]);
    write_end.Reset(descriptors[1]);
    if (!RaiseDescriptor(read_end, error) ||
        !RaiseDescriptor(write_end, error))
    {
        error = "preparing a pipe: " + error;
        return false;
    }
    return true;
}

bool CreateTemporaryFile(FileDescriptor& file, std::string& error)
{
    std::error_code path_error;
    const std::filesystem::path directory =
        std::filesystem::temp_directory_path(path_error);
    if (path_error)
    {
        error = "finding the temporary directory: " + path_error.message();
        return false;
    }

    std::string pattern = (directory / "nuocc-command-XXXXXX").string();
    std::vector<char> path(pattern.begin(), pattern.end());
    path.push_back('\0');

    FileDescriptor descriptor(mkostemp(path.data(), O_CLOEXEC));
    if (descriptor.Get() == -1)
    {
        error = "creating the temporary input: " +
            std::string(std::strerror(errno));
        return false;
    }
    if (unlink(path.data()) != 0)
    {
        error = "removing the temporary input: " +
            std::string(std::strerror(errno));
        std::error_code cleanup_error;
        std::filesystem::remove(path.data(), cleanup_error);
        return false;
    }
    if (!RaiseDescriptor(descriptor, error))
    {
        error = "preparing the temporary input: " + error;
        return false;
    }

    file = std::move(descriptor);
    return true;
}

bool WriteAll(int descriptor, const char* data, std::size_t size,
    std::string& error)
{
    while (size > 0)
    {
        const ssize_t count = write(descriptor, data, size);
        if (count > 0)
        {
            data += count;
            size -= static_cast<std::size_t>(count);
            continue;
        }
        if (count == -1 && errno == EINTR)
            continue;
        if (count == 0)
        {
            error = "writing the temporary input made no progress";
            return false;
        }

        error = std::strerror(errno);
        return false;
    }
    return true;
}

bool BuildInput(const ToolCommand& command, FileDescriptor& input,
    ToolErrorCode& code, std::string& error)
{
    code = ToolErrorCode::kInput;
    if (!CreateTemporaryFile(input, error))
        return false;

    char buffer[4096];
    for (const std::string& path_name : command.input_files)
    {
        FileDescriptor source(open(path_name.c_str(), O_RDONLY | O_CLOEXEC));
        if (source.Get() == -1)
        {
            code = ToolErrorCode::kInput;
            error = path_name + ": " + std::strerror(errno);
            return false;
        }

        while (true)
        {
            const ssize_t count = read(source.Get(), buffer, sizeof(buffer));
            if (count > 0)
            {
                if (!WriteAll(input.Get(), buffer,
                        static_cast<std::size_t>(count), error))
                {
                    code = ToolErrorCode::kInput;
                    error = "writing the temporary input: " + error;
                    return false;
                }
                continue;
            }
            if (count == 0)
                break;
            if (errno == EINTR)
                continue;

            code = ToolErrorCode::kInput;
            error = path_name + ": " + std::strerror(errno);
            return false;
        }
    }

    if (lseek(input.Get(), 0, SEEK_SET) == -1)
    {
        code = ToolErrorCode::kInput;
        error = "rewinding the temporary input: " +
            std::string(std::strerror(errno));
        return false;
    }
    return true;
}

class FileActions
{
public:
    FileActions()
    {
        error_ = posix_spawn_file_actions_init(&actions_);
        initialized_ = error_ == 0;
    }

    ~FileActions()
    {
        if (initialized_)
            posix_spawn_file_actions_destroy(&actions_);
    }

    FileActions(const FileActions&) = delete;
    FileActions& operator=(const FileActions&) = delete;

    posix_spawn_file_actions_t* Get()
    {
        return &actions_;
    }

    int GetError() const
    {
        return error_;
    }

    void Close(int descriptor)
    {
        if (error_ == 0)
            error_ = posix_spawn_file_actions_addclose(&actions_, descriptor);
    }

    void Duplicate(int descriptor, int destination)
    {
        if (error_ == 0)
        {
            error_ = posix_spawn_file_actions_adddup2(
                &actions_, descriptor, destination);
        }
    }

    void Open(int descriptor, const char* path, int flags)
    {
        if (error_ == 0)
        {
            error_ = posix_spawn_file_actions_addopen(
                &actions_, descriptor, path, flags, 0);
        }
    }

private:
    posix_spawn_file_actions_t actions_;
    int error_ = 0;
    bool initialized_ = false;
};

class SpawnAttributes
{
public:
    SpawnAttributes()
    {
        error_ = posix_spawnattr_init(&attributes_);
        initialized_ = error_ == 0;
    }

    ~SpawnAttributes()
    {
        if (initialized_)
            posix_spawnattr_destroy(&attributes_);
    }

    SpawnAttributes(const SpawnAttributes&) = delete;
    SpawnAttributes& operator=(const SpawnAttributes&) = delete;

    posix_spawnattr_t* Get()
    {
        return &attributes_;
    }

    int GetError() const
    {
        return error_;
    }

    void StartNewProcessGroup()
    {
        if (error_ == 0)
            error_ = posix_spawnattr_setflags(&attributes_,
                POSIX_SPAWN_SETPGROUP);
        if (error_ == 0)
            error_ = posix_spawnattr_setpgroup(&attributes_, 0);
    }

private:
    posix_spawnattr_t attributes_;
    int error_ = 0;
    bool initialized_ = false;
};

bool SetNonBlocking(int descriptor, std::string& error)
{
    const int flags = fcntl(descriptor, F_GETFL);
    if (flags != -1 && fcntl(descriptor, F_SETFL, flags | O_NONBLOCK) != -1)
        return true;

    error = std::strerror(errno);
    return false;
}

int Kill(pid_t child)
{
    int result = kill(-child, SIGKILL);
    if (result == -1 && errno == ESRCH)
        result = kill(child, SIGKILL);
    return result;
}

bool Reap(pid_t child, int& status, bool& reaped, std::string& error)
{
    pid_t waited;
    do
    {
        waited = waitpid(child, &status, 0);
    }
    while (waited == -1 && errno == EINTR);

    if (waited != -1)
    {
        reaped = true;
        return true;
    }
    if (errno == ECHILD)
    {
        reaped = true;
        return false;
    }
    error = std::strerror(errno);
    return false;
}

bool WaitWithTimeout(pid_t child,
    std::chrono::steady_clock::time_point deadline, int& status, bool& reaped,
    bool& timed_out, std::string& error)
{
    timed_out = false;
    while (true)
    {
        const pid_t waited = waitpid(child, &status, WNOHANG);
        if (waited == child)
        {
            reaped = true;
            return true;
        }
        if (waited == -1)
        {
            if (errno == EINTR)
                continue;
            const int wait_error = errno;
            Kill(child);
            if (wait_error == ECHILD)
                reaped = true;
            else
                Reap(child, status, reaped, error);
            error = std::strerror(wait_error);
            return false;
        }

        if (std::chrono::steady_clock::now() >= deadline)
        {
            const int killed = Kill(child);
            if (killed == -1 && errno == ESRCH)
                return Reap(child, status, reaped, error);
            timed_out = true;
            return Reap(child, status, reaped, error);
        }
        usleep(10000);
    }
}

bool Drain(int descriptor, std::optional<std::string>& text,
    std::size_t& budget, bool& limited, bool& ended, std::string& error)
{
    if (!text)
        text.emplace();
    char buffer[4096];
    while (true)
    {
        const ssize_t count = read(descriptor, buffer, sizeof(buffer));
        if (count > 0)
        {
            const std::size_t size = static_cast<std::size_t>(count);
            const std::size_t copied = std::min(size, budget);
            text->append(buffer, copied);
            budget -= copied;
            if (copied != size)
            {
                limited = true;
                return true;
            }
            continue;
        }
        if (count == 0)
        {
            ended = true;
            return true;
        }
        if (errno == EINTR)
            continue;
        if (errno == EAGAIN || errno == EWOULDBLOCK)
            return true;

        error = std::strerror(errno);
        return false;
    }
}

bool Capture(pid_t child, int output_descriptor, int error_descriptor,
    std::size_t maximum_output,
    std::chrono::steady_clock::time_point deadline, int& status,
    std::optional<std::string>& standard_output,
    std::optional<std::string>& standard_error, bool& output_limited,
    bool& reaped, bool& timed_out, std::string& error)
{
    status = 0;
    output_limited = false;
    timed_out = false;
    if (!SetNonBlocking(output_descriptor, error) ||
        !SetNonBlocking(error_descriptor, error))
    {
        Kill(child);
        Reap(child, status, reaped, error);
        return false;
    }

    std::size_t budget = maximum_output;
    bool output_ended = false;
    bool error_ended = false;
    bool child_exited = false;

    while (true)
    {
        if (!output_ended &&
            !Drain(output_descriptor, standard_output, budget, output_limited,
                output_ended, error))
        {
            if (!child_exited)
            {
                Kill(child);
                Reap(child, status, reaped, error);
            }
            return false;
        }
        if (!error_ended &&
            !Drain(error_descriptor, standard_error, budget, output_limited,
                error_ended, error))
        {
            if (!child_exited)
            {
                Kill(child);
                Reap(child, status, reaped, error);
            }
            return false;
        }

        if (output_limited)
        {
            if (!child_exited)
            {
                Kill(child);
                if (!Reap(child, status, reaped, error))
                    return false;
            }
            return true;
        }
        if (child_exited)
            return true;

        if (!child_exited)
        {
            const pid_t waited = waitpid(child, &status, WNOHANG);
            if (waited == child)
            {
                child_exited = true;
                reaped = true;
                continue;
            }
            if (waited == -1)
            {
                if (errno == EINTR)
                    continue;
                const int wait_error = errno;
                Kill(child);
                if (wait_error == ECHILD)
                    reaped = true;
                else
                    Reap(child, status, reaped, error);
                error = std::strerror(wait_error);
                return false;
            }
        }

        if (std::chrono::steady_clock::now() >= deadline)
        {
            const int killed = Kill(child);
            if (killed == -1 && errno == ESRCH)
                return Reap(child, status, reaped, error);
            timed_out = true;
            return Reap(child, status, reaped, error);
        }

        pollfd descriptors[2] = {
            {.fd = output_ended ? -1 : output_descriptor,
                .events = POLLIN, .revents = 0},
            {.fd = error_ended ? -1 : error_descriptor,
                .events = POLLIN, .revents = 0},
        };
        const int ready = poll(descriptors, 2, 10);
        if (ready == -1 && errno != EINTR)
        {
            error = std::strerror(errno);
            if (!child_exited)
            {
                Kill(child);
                Reap(child, status, reaped, error);
            }
            return false;
        }
    }
}

}   /* namespace */

std::string Quote(const std::string& text)
{
    std::string quoted = "'";
    for (char character : text)
    {
        if (character == '\'')
            quoted += "'\\''";
        else
            quoted += character;
    }
    return quoted + "'";
}

std::string FormatCommand(const ToolCommand& command)
{
    std::string inputs;
    for (const std::string& path : command.input_files)
        inputs += (inputs.empty() ? "" : " ") + Quote(path);

    std::string arguments;
    for (const std::string& argument : command.arguments)
        arguments += (arguments.empty() ? "" : " ") + Quote(argument);

    if (inputs.empty())
        return arguments;
    if (arguments.empty())
        return "<invalid command> with input from " + inputs;
    return "cat " + inputs + " | " + arguments;
}

ToolResult RunTool(const ToolCommand& command, bool capture_output,
    std::size_t maximum_output, std::chrono::milliseconds timeout)
{
    if (command.arguments.empty())
        return Failure(ToolErrorCode::kInvalidCommand, "the command is empty");

    for (const std::string& argument : command.arguments)
    {
        if (argument.find('\0') != std::string::npos)
            return Failure(ToolErrorCode::kInvalidCommand,
                "an argument contains an embedded NUL byte");
    }
    for (const std::string& path : command.input_files)
    {
        if (path.find('\0') != std::string::npos)
            return Failure(ToolErrorCode::kInput,
                "an input path contains an embedded NUL byte");
    }

    if (timeout <= std::chrono::milliseconds::zero())
        return Failure(ToolErrorCode::kTimeout, "the timeout expired");

    FileDescriptor input;
    if (!command.input_files.empty())
    {
        std::string error;
        ToolErrorCode code = ToolErrorCode::kInput;
        if (!BuildInput(command, input, code, error))
            return Failure(code, std::move(error));
    }

    const std::chrono::steady_clock::time_point deadline =
        std::chrono::steady_clock::now() + timeout;
    FileDescriptor output_read;
    FileDescriptor output_write;
    FileDescriptor error_read;
    FileDescriptor error_write;
    if (capture_output)
    {
        std::string error;
        if (!CreatePipe(output_read, output_write, error) ||
            !CreatePipe(error_read, error_write, error))
        {
            return Failure(ToolErrorCode::kStart, std::move(error));
        }
    }

    FileActions actions;
    if (actions.GetError() != 0)
        return Failure(ToolErrorCode::kStart, std::strerror(actions.GetError()));

    SpawnAttributes attributes;
    attributes.StartNewProcessGroup();
    if (attributes.GetError() != 0)
    {
        return Failure(
            ToolErrorCode::kStart, std::strerror(attributes.GetError()));
    }

    if (input.Get() != -1)
    {
        actions.Duplicate(input.Get(), STDIN_FILENO);
        actions.Close(input.Get());
    }
    else
    {
        actions.Open(STDIN_FILENO, "/dev/null", O_RDONLY);
    }

    if (capture_output)
    {
        actions.Duplicate(output_write.Get(), STDOUT_FILENO);
        actions.Close(output_write.Get());
        actions.Close(output_read.Get());
        actions.Duplicate(error_write.Get(), STDERR_FILENO);
        actions.Close(error_write.Get());
        actions.Close(error_read.Get());
    }
    if (actions.GetError() != 0)
        return Failure(ToolErrorCode::kStart, std::strerror(actions.GetError()));

    std::vector<char*> arguments;
    arguments.reserve(command.arguments.size() + 1);
    for (const std::string& argument : command.arguments)
        arguments.push_back(const_cast<char*>(argument.c_str()));
    arguments.push_back(nullptr);

    pid_t child = -1;
    const int spawn_error = posix_spawnp(&child, arguments.front(),
        actions.Get(), attributes.Get(), arguments.data(), environ);
    if (spawn_error != 0)
        return Failure(ToolErrorCode::kStart, std::strerror(spawn_error));

    output_write.Reset();
    error_write.Reset();

    bool child_reaped = false;
    try
    {
        int status = 0;
        std::optional<std::string> standard_output;
        std::optional<std::string> standard_error;
        bool output_limited = false;
        bool timed_out = false;
        std::string error;
        bool succeeded = true;
        if (capture_output)
        {
            succeeded = Capture(child, output_read.Get(), error_read.Get(),
                maximum_output, deadline, status, standard_output,
                standard_error, output_limited, child_reaped, timed_out, error);
        }
        else
        {
            succeeded = WaitWithTimeout(
                child, deadline, status, child_reaped, timed_out, error);
        }

        ToolResult result;
        if (succeeded)
            result = ClassifyStatus(status);
        result.standard_output = std::move(standard_output);
        result.standard_error = std::move(standard_error);
        if (!succeeded)
        {
            result.error = capture_output ?
                ToolErrorCode::kRead : ToolErrorCode::kWait;
            result.error_message = std::move(error);
        }
        else if (output_limited)
        {
            result.error = ToolErrorCode::kOutputLimit;
            result.error_message = "captured output exceeded " +
                std::to_string(maximum_output) + " bytes";
        }
        else if (timed_out)
        {
            result.error = ToolErrorCode::kTimeout;
            result.error_message = "the tool did not finish before the timeout";
        }
        return result;
    }
    catch (...)
    {
        if (!child_reaped)
        {
            Kill(child);
            int ignored_status = 0;
            while (waitpid(child, &ignored_status, 0) == -1 && errno == EINTR)
            {
            }
        }
        throw;
    }
}

}   /* namespace nuocc */
