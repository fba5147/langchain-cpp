#pragma once

// Abstracts how McpClient exchanges already-framed JSON-RPC messages with a
// server, so McpClient itself (id bookkeeping, initialize handshake,
// tools/list, tools/call) doesn't need to know whether it's talking to a
// subprocess over stdio or a remote server over HTTP.

#include <nlohmann/json.hpp>

namespace langchain::mcp::detail {

class ClientTransport {
public:
    virtual ~ClientTransport() = default;

    // `request` is a fully-built JSON-RPC request (jsonrpc/id/method/params,
    // see jsonrpc.hpp's build_request). Returns the response's "result" on
    // success; throws std::runtime_error with the server's own error
    // message if the response is a JSON-RPC error, or if the connection is
    // lost before a response arrives.
    virtual nlohmann::json send_request(const nlohmann::json& request) = 0;

    // `notification` is a fully-built JSON-RPC notification (no "id"). No
    // response is expected.
    virtual void send_notification(const nlohmann::json& notification) = 0;
};

} // namespace langchain::mcp::detail
