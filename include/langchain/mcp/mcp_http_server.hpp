#pragma once

#include "langchain/mcp/mcp_server.hpp"

#include <atomic>
#include <string>
#include <thread>

namespace langchain::mcp {

// Serves a ToolRegistry over MCP's "Streamable HTTP" transport (see
// McpHttpConfig on the client side): binds to host:port and, on a
// background thread, accepts POST requests at `path`, dispatching each
// through the same request handling McpServer::handle_message() uses for
// stdio, and always answering with a single `Content-Type:
// application/json` response. That's spec-compliant -- servers may also
// open an SSE stream for a request, but this server never needs to push
// more than one message per request, so there's nothing to gain from the
// extra complexity. GET requests get 405 Method Not Allowed, since this
// server never pushes unsolicited messages and so has no use for the
// standalone server-to-client stream the spec allows for that.
//
// Deliberately minimal, same spirit as StdioTransport/McpServer's stdio
// path: single connection at a time, no TLS, no session ID issuance (the
// spec makes that optional), no resumability. Requests carrying an
// `Origin` header naming something other than a `localhost`/`127.0.0.1`
// origin are rejected -- a real, if minimal, version of the spec's
// DNS-rebinding mitigation; requests with no `Origin` header at all (e.g.
// from cpr, curl, or any other non-browser client, which is what
// McpClient's own HttpClientTransport is) are let through, since this
// server has no cross-origin *browser* use case to protect against.
class McpHttpServer {
public:
    // port=0 asks the OS for a free port -- see port(). Throws
    // std::runtime_error if the socket can't be bound.
    explicit McpHttpServer(std::shared_ptr<tools::ToolRegistry> registry, std::string host = "127.0.0.1",
                            int port = 0, std::string path = "/mcp");
    ~McpHttpServer();

    McpHttpServer(const McpHttpServer&) = delete;
    McpHttpServer& operator=(const McpHttpServer&) = delete;

    int port() const;
    // e.g. "http://127.0.0.1:54321/mcp" -- pass to McpHttpConfig::url.
    std::string url() const;

private:
    void run();

    McpServer server_;
    std::string host_;
    std::string path_;
    int listen_fd_ = -1;
    int port_ = 0;
    std::atomic<bool> stop_{false};
    std::thread thread_;
};

} // namespace langchain::mcp
