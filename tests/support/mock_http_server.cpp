#include "mock_http_server.hpp"

#include <algorithm>
#include <cctype>
#include <cstring>
#include <stdexcept>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <poll.h>
#include <sys/socket.h>
#include <unistd.h>

namespace langchain::testing {

namespace {

std::string to_lower(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char c) { return std::tolower(c); });
    return value;
}

std::string trim(const std::string& value) {
    std::size_t start = value.find_first_not_of(" \t\r\n");
    if (start == std::string::npos) {
        return "";
    }
    std::size_t end = value.find_last_not_of(" \t\r\n");
    return value.substr(start, end - start + 1);
}

const char* reason_phrase(int status) {
    switch (status) {
        case 200:
            return "OK";
        case 400:
            return "Bad Request";
        case 401:
            return "Unauthorized";
        case 404:
            return "Not Found";
        case 429:
            return "Too Many Requests";
        case 500:
            return "Internal Server Error";
        default:
            return "Error";
    }
}

// Reads exactly one HTTP/1.1 request off `fd`: the request line, headers
// up to the blank line, then the body (sized by Content-Length; 0 if
// absent, which is all a JSON-POSTing client like cpr ever omits it for).
MockHttpRequest read_request(int fd) {
    std::string buffer;
    char chunk[4096];

    auto header_end = std::string::npos;
    while ((header_end = buffer.find("\r\n\r\n")) == std::string::npos) {
        ssize_t n = recv(fd, chunk, sizeof(chunk), 0);
        if (n <= 0) {
            throw std::runtime_error("MockHttpServer: connection closed before headers were complete");
        }
        buffer.append(chunk, static_cast<std::size_t>(n));
    }

    std::string head = buffer.substr(0, header_end);
    std::string body_so_far = buffer.substr(header_end + 4);

    std::size_t line_end = head.find("\r\n");
    std::string request_line = head.substr(0, line_end);

    MockHttpRequest request;
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
            request.headers[to_lower(trim(line.substr(0, colon)))] = trim(line.substr(colon + 1));
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
            throw std::runtime_error("MockHttpServer: connection closed before body was complete");
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
            throw std::runtime_error("MockHttpServer: send() failed");
        }
        sent += static_cast<std::size_t>(n);
    }
}

void write_response(int fd, const MockHttpResponse& response) {
    std::string message = "HTTP/1.1 " + std::to_string(response.status) + " " + reason_phrase(response.status) +
                           "\r\n" + "Content-Type: " + response.content_type + "\r\n" +
                           "Content-Length: " + std::to_string(response.body.size()) + "\r\n" + "Connection: close\r\n";
    for (const auto& [name, value] : response.extra_headers) {
        message += name + ": " + value + "\r\n";
    }
    message += "\r\n" + response.body;
    write_all(fd, message);
}

} // namespace

std::string MockHttpRequest::header(const std::string& name) const {
    auto it = headers.find(to_lower(name));
    return it != headers.end() ? it->second : "";
}

MockHttpServer::MockHttpServer(Handler handler) : handler_(std::move(handler)) {
    listen_fd_ = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_fd_ < 0) {
        throw std::runtime_error("MockHttpServer: socket() failed");
    }

    int reuse = 1;
    setsockopt(listen_fd_, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));

    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = inet_addr("127.0.0.1");
    address.sin_port = 0; // ask the OS for a free port

    if (bind(listen_fd_, reinterpret_cast<sockaddr*>(&address), sizeof(address)) != 0) {
        close(listen_fd_);
        throw std::runtime_error("MockHttpServer: bind() failed");
    }
    if (listen(listen_fd_, /*backlog=*/8) != 0) {
        close(listen_fd_);
        throw std::runtime_error("MockHttpServer: listen() failed");
    }

    sockaddr_in bound{};
    socklen_t bound_len = sizeof(bound);
    getsockname(listen_fd_, reinterpret_cast<sockaddr*>(&bound), &bound_len);
    port_ = ntohs(bound.sin_port);

    thread_ = std::thread(&MockHttpServer::run, this);
}

MockHttpServer::~MockHttpServer() {
    stop_ = true;
    if (thread_.joinable()) {
        thread_.join();
    }
    if (listen_fd_ >= 0) {
        close(listen_fd_);
    }
}

std::string MockHttpServer::base_url() const { return "http://127.0.0.1:" + std::to_string(port_); }

void MockHttpServer::run() {
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
            MockHttpRequest request = read_request(client_fd);
            MockHttpResponse response = handler_(request);
            write_response(client_fd, response);
        } catch (const std::exception&) {
            // A malformed request or a handler that threw shouldn't take
            // the whole server thread down mid-test; the test itself will
            // see the failure (e.g. via a missing/unexpected response).
        }
        close(client_fd);
    }
}

} // namespace langchain::testing
