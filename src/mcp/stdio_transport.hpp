#pragma once

// Spawns a child process and exchanges newline-delimited messages over
// its stdin/stdout -- the framing MCP's stdio transport uses (one JSON
// value per line, UTF-8, no embedded newlines; unlike LSP, there's no
// Content-Length header). POSIX-only (fork/exec/pipe): the project's CI
// only targets ubuntu-latest/macos-latest, so a Windows path (CreateProcess
// etc.) isn't implemented.

#include <optional>
#include <string>
#include <sys/types.h>
#include <vector>

namespace langchain::mcp::detail {

class StdioTransport {
public:
    // `command[0]` is the executable, resolved via PATH like execvp; the
    // rest are its arguments. Throws std::runtime_error if the pipes or
    // the child process can't be created.
    explicit StdioTransport(std::vector<std::string> command);
    ~StdioTransport();

    StdioTransport(const StdioTransport&) = delete;
    StdioTransport& operator=(const StdioTransport&) = delete;

    // Appends a newline and writes to the child's stdin. Throws
    // std::runtime_error if the write fails (e.g. the child already exited).
    void write_line(const std::string& line);

    // Blocks until a full line is available from the child's stdout.
    // Returns std::nullopt once the child closes its output (normally
    // because it exited).
    std::optional<std::string> read_line();

private:
    pid_t child_pid_ = -1;
    int stdin_fd_ = -1;
    int stdout_fd_ = -1;
    std::string read_buffer_;
};

} // namespace langchain::mcp::detail
