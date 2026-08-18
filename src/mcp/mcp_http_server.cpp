#include "langchain/mcp/mcp_http_server.hpp"

#include <arpa/inet.h>
#include <netinet/in.h>
#include <poll.h>
#include <sys/socket.h>
#include <unistd.h>

#include <algorithm>
#include <cctype>
#include <map>
#include <stdexcept>

namespace langchain::mcp {

using json = nlohmann::json;

namespace {

std::string to_lower(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char c) { return std::tolower(c); });
    return value;
}

struct RawRequest {
    std::string method;
    std::string path;
    std::map<std::string, std::string> headers; // keys lowercased
    std::string body;
};

RawRequest read_raw_request(int fd) {
    std::string buffer;
    char chunk[4096];

    std::size_t header_end;
    while ((header_end = buffer.find("\r\n\r\n")) == std::string::npos) {
        ssize_t n = recv(fd, chunk, sizeof(chunk), 0);
        if (n <= 0) {
            throw std::runtime_error("McpHttpServer: connection closed before headers were complete");
        }
        buffer.append(chunk, static_cast<std::size_t>(n));
    }

    std::string head = buffer.substr(0, header_end);
    std::string body_so_far = buffer.substr(header_end + 4);

    std::size_t line_end = head.find("\r\n");
    std::string request_line = head.substr(0, line_end);

    RawRequest request;
    std::size_t method_end = request_line.find(' ');
    request.method = request_line.substr(0, method_end);
    std::size_t path_end = request_line.find(' ', method_end + 1);
    request.path = request_line.substr(method_end + 1, path_end - method_end - 1);

    std::size_t pos = line_end + 2;
    while (pos < head.size()) {
        std::size_t next = head.find("\r\n", pos);
        if (next == std::string::npos) {
            next = head.size();
        }
        std::string line = head.substr(pos, next - pos);
        std::size_t colon = line.find(':');
        if (colon != std::string::npos) {
            std::string name = to_lower(line.substr(0, colon));
            std::string value = line.substr(colon + 1);
            std::size_t start = value.find_first_not_of(" \t");
            request.headers[name] = (start == std::string::npos) ? "" : value.substr(start);
        }
        pos = next + 2;
    }

    std::size_t content_length = 0;
    auto it = request.headers.find("content-length");
    if (it != request.headers.end()) {
        content_length = static_cast<std::size_t>(std::stoul(it->second));
    }

    std::string body = body_so_far;
    while (body.size() < content_length) {
        ssize_t n = recv(fd, chunk, sizeof(chunk), 0);
        if (n <= 0) {
            throw std::runtime_error("McpHttpServer: connection closed before body was complete");
        }
        body.append(chunk, static_cast<std::size_t>(n));
    }
    request.body = body.substr(0, content_length);
    return request;
}

void write_all(int fd, const std::string& data) {
    std::size_t sent = 0;
    while (sent < data.size()) {
        ssize_t n = send(fd, data.data() + sent, data.size() - sent, 0);
        if (n < 0) {
            throw std::runtime_error("McpHttpServer: send() failed");
        }
        sent += static_cast<std::size_t>(n);
    }
}

void write_raw_response(int fd, int status, const std::string& reason, const std::string& body,
                         const std::string& content_type = "application/json") {
    std::string message = "HTTP/1.1 " + std::to_string(status) + " " + reason + "\r\n" +
                           "Content-Type: " + content_type + "\r\n" +
                           "Content-Length: " + std::to_string(body.size()) + "\r\n" + "Connection: close\r\n\r\n" +
                           body;
    write_all(fd, message);
}

bool origin_is_local(const std::string& origin) {
    static const char* allowed_prefixes[] = {"http://localhost", "http://127.0.0.1", "https://localhost",
                                              "https://127.0.0.1"};
    for (const char* prefix : allowed_prefixes) {
        if (origin.rfind(prefix, 0) == 0) {
            return true;
        }
    }
    return false;
}

} // namespace

McpHttpServer::McpHttpServer(std::shared_ptr<tools::ToolRegistry> registry, std::string host, int port,
                              std::string path)
    : server_(std::move(registry)), host_(std::move(host)), path_(std::move(path)) {
    listen_fd_ = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_fd_ < 0) {
        throw std::runtime_error("McpHttpServer: socket() failed");
    }

    int reuse = 1;
    setsockopt(listen_fd_, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));

    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = inet_addr(host_.c_str());
    address.sin_port = htons(static_cast<std::uint16_t>(port));

    if (bind(listen_fd_, reinterpret_cast<sockaddr*>(&address), sizeof(address)) != 0) {
        close(listen_fd_);
        throw std::runtime_error("McpHttpServer: bind() failed");
    }
    if (listen(listen_fd_, /*backlog=*/8) != 0) {
        close(listen_fd_);
        throw std::runtime_error("McpHttpServer: listen() failed");
    }

    sockaddr_in bound{};
    socklen_t bound_len = sizeof(bound);
    getsockname(listen_fd_, reinterpret_cast<sockaddr*>(&bound), &bound_len);
    port_ = ntohs(bound.sin_port);

    thread_ = std::thread(&McpHttpServer::run, this);
}

McpHttpServer::~McpHttpServer() {
    stop_ = true;
    if (thread_.joinable()) {
        thread_.join();
    }
    if (listen_fd_ >= 0) {
        close(listen_fd_);
    }
}

int McpHttpServer::port() const { return port_; }

std::string McpHttpServer::url() const { return "http://" + host_ + ":" + std::to_string(port_) + path_; }

void McpHttpServer::run() {
    while (!stop_) {
        pollfd poll_fd{listen_fd_, POLLIN, 0};
        int ready = poll(&poll_fd, 1, /*timeout_ms=*/100);
        if (ready <= 0) {
            continue; // timeout (check stop_ again) or a transient poll error
        }

        int client_fd = accept(listen_fd_, nullptr, nullptr);
        if (client_fd < 0) {
            continue;
        }

        try {
            RawRequest request = read_raw_request(client_fd);

            auto origin_it = request.headers.find("origin");
            if (origin_it != request.headers.end() && !origin_is_local(origin_it->second)) {
                write_raw_response(client_fd, 403, "Forbidden", "");
            } else if (request.path != path_) {
                write_raw_response(client_fd, 404, "Not Found", "");
            } else if (request.method != "POST") {
                write_raw_response(client_fd, 405, "Method Not Allowed", "");
            } else {
                json message;
                bool parsed_ok = true;
                try {
                    message = json::parse(request.body);
                } catch (const json::parse_error&) {
                    parsed_ok = false;
                }

                if (!parsed_ok) {
                    write_raw_response(client_fd, 400, "Bad Request", "");
                } else {
                    auto response = server_.handle_message(message);
                    if (!response.has_value()) {
                        write_raw_response(client_fd, 202, "Accepted", "");
                    } else {
                        write_raw_response(client_fd, 200, "OK", response->dump());
                    }
                }
            }
        } catch (const std::exception&) {
            // A malformed request or a transient socket error shouldn't
            // take the whole server thread down -- the client just sees a
            // dropped connection and can retry.
        }
        close(client_fd);
    }
}

} // namespace langchain::mcp
