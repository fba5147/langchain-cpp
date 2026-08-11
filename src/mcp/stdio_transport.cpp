#include "stdio_transport.hpp"

#include <cerrno>
#include <chrono>
#include <cstring>
#include <stdexcept>
#include <thread>

#include <signal.h>
#include <sys/wait.h>
#include <unistd.h>

namespace langchain::mcp::detail {

namespace {

void close_if_open(int& fd) {
    if (fd >= 0) {
        close(fd);
        fd = -1;
    }
}

} // namespace

StdioTransport::StdioTransport(std::vector<std::string> command) {
    if (command.empty()) {
        throw std::runtime_error("StdioTransport: command must have at least one element (the executable)");
    }

    int stdin_pipe[2];
    int stdout_pipe[2];
    if (pipe(stdin_pipe) != 0 || pipe(stdout_pipe) != 0) {
        throw std::runtime_error(std::string("StdioTransport: pipe() failed: ") + std::strerror(errno));
    }

    pid_t pid = fork();
    if (pid < 0) {
        close(stdin_pipe[0]);
        close(stdin_pipe[1]);
        close(stdout_pipe[0]);
        close(stdout_pipe[1]);
        throw std::runtime_error(std::string("StdioTransport: fork() failed: ") + std::strerror(errno));
    }

    if (pid == 0) {
        // Child: wire the read end of stdin_pipe to our stdin, and the
        // write end of stdout_pipe to our stdout, then exec.
        dup2(stdin_pipe[0], STDIN_FILENO);
        dup2(stdout_pipe[1], STDOUT_FILENO);
        close(stdin_pipe[0]);
        close(stdin_pipe[1]);
        close(stdout_pipe[0]);
        close(stdout_pipe[1]);

        std::vector<char*> argv;
        argv.reserve(command.size() + 1);
        for (auto& arg : command) {
            argv.push_back(arg.data());
        }
        argv.push_back(nullptr);

        execvp(argv[0], argv.data());
        _exit(127); // execvp only returns on failure
    }

    // Parent: keep the write end of stdin_pipe and the read end of
    // stdout_pipe; the other two ends belong to the child now.
    close(stdin_pipe[0]);
    close(stdout_pipe[1]);
    child_pid_ = pid;
    stdin_fd_ = stdin_pipe[1];
    stdout_fd_ = stdout_pipe[0];
}

StdioTransport::~StdioTransport() {
    // Closing stdin first lets a well-behaved server see EOF and exit on
    // its own; if it doesn't within a short grace period, kill it rather
    // than block a destructor indefinitely.
    close_if_open(stdin_fd_);
    close_if_open(stdout_fd_);

    if (child_pid_ > 0) {
        bool exited = false;
        for (int i = 0; i < 25 && !exited; ++i) {
            int status;
            pid_t result = waitpid(child_pid_, &status, WNOHANG);
            if (result == child_pid_) {
                exited = true;
            } else {
                std::this_thread::sleep_for(std::chrono::milliseconds(20));
            }
        }
        if (!exited) {
            kill(child_pid_, SIGKILL);
            waitpid(child_pid_, nullptr, 0);
        }
    }
}

void StdioTransport::write_line(const std::string& line) {
    std::string data = line + "\n";
    std::size_t written = 0;
    while (written < data.size()) {
        ssize_t n = write(stdin_fd_, data.data() + written, data.size() - written);
        if (n < 0) {
            if (errno == EINTR) {
                continue;
            }
            throw std::runtime_error(std::string("StdioTransport: write() failed: ") + std::strerror(errno));
        }
        written += static_cast<std::size_t>(n);
    }
}

std::optional<std::string> StdioTransport::read_line() {
    while (true) {
        auto newline_pos = read_buffer_.find('\n');
        if (newline_pos != std::string::npos) {
            std::string line = read_buffer_.substr(0, newline_pos);
            read_buffer_.erase(0, newline_pos + 1);
            if (!line.empty() && line.back() == '\r') {
                line.pop_back();
            }
            return line;
        }

        char chunk[4096];
        ssize_t n = read(stdout_fd_, chunk, sizeof(chunk));
        if (n < 0) {
            if (errno == EINTR) {
                continue;
            }
            throw std::runtime_error(std::string("StdioTransport: read() failed: ") + std::strerror(errno));
        }
        if (n == 0) {
            // EOF: the child closed its stdout (usually because it exited).
            if (!read_buffer_.empty()) {
                std::string line = read_buffer_;
                read_buffer_.clear();
                return line;
            }
            return std::nullopt;
        }
        read_buffer_.append(chunk, static_cast<std::size_t>(n));
    }
}

} // namespace langchain::mcp::detail
