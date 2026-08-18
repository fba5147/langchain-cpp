#pragma once

// A minimal HTTP/1.1 server for tests: binds to 127.0.0.1 on an
// OS-assigned port and runs a handler on a background thread, so a real
// ChatModel (via cpr, real sockets, real HTTP parsing on both ends) can
// be pointed at it through its `base_url` config -- exercising the full
// request/response path (URL construction, headers, JSON body, status
// codes) rather than just the pure conversion functions those providers'
// wire-format modules already unit-test.
//
// Deliberately minimal: single connection at a time, no TLS, no chunked
// transfer encoding, no keep-alive. That's everything a ChatModel's
// blocking, one-shot `cpr::Post` needs and nothing more.

#include <atomic>
#include <functional>
#include <map>
#include <string>
#include <thread>

namespace langchain::testing {

struct MockHttpRequest {
    std::string method;
    std::string path; // includes any query string
    std::map<std::string, std::string> headers; // keys lowercased
    std::string body;

    // Case-insensitive lookup; returns "" if absent.
    std::string header(const std::string& name) const;
};

struct MockHttpResponse {
    int status = 200;
    std::string body;
    std::string content_type = "application/json";
    // Extra response headers beyond Content-Type/Content-Length/Connection
    // (which the server always sets itself) -- e.g. Mcp-Session-Id for the
    // MCP Streamable HTTP contract tests.
    std::map<std::string, std::string> extra_headers;
};

class MockHttpServer {
public:
    using Handler = std::function<MockHttpResponse(const MockHttpRequest&)>;

    explicit MockHttpServer(Handler handler);
    ~MockHttpServer();

    MockHttpServer(const MockHttpServer&) = delete;
    MockHttpServer& operator=(const MockHttpServer&) = delete;

    // e.g. "http://127.0.0.1:54321" -- pass as a provider config's base_url.
    std::string base_url() const;

private:
    void run();

    int listen_fd_ = -1;
    int port_ = 0;
    Handler handler_;
    std::atomic<bool> stop_{false};
    std::thread thread_;
};

} // namespace langchain::testing
