#pragma once

// ClientTransport implementation of MCP's "Streamable HTTP" transport (the
// transport that replaced the older separate HTTP+SSE transport in the
// 2025-03-26 spec revision -- see
// modelcontextprotocol.io/specification/2025-06-18/basic/transports).
// Verified against a real reference server (`npx
// @modelcontextprotocol/server-everything streamableHttp`), not just the
// spec text.
//
// Scope: this client only ever sends one JSON-RPC request per HTTP POST and
// reads that request's own response (whether the server answers with
// `Content-Type: application/json` or opens a `text/event-stream` for it) --
// it does not open a standalone GET stream for unsolicited server-to-client
// requests/notifications, since McpClient has no use for those today (same
// scope limitation the stdio transport already has). Session ID handling
// (`Mcp-Session-Id`) is supported since real servers (including the
// reference server above) rely on it; resumability (SSE event `Last-Event-
// ID` redelivery) is not, since a lost connection here just surfaces as a
// failed request -- there's nothing mid-flight to resume.

#include "client_transport.hpp"

#include <optional>
#include <string>

namespace langchain::mcp::detail {

// Extracts the JSON-RPC message from an SSE-formatted response body: joins
// multi-line "data:" fields within one event with "\n" (per the SSE spec),
// and returns the *last* event's data if the body contains more than one
// (earlier events would be interim server requests/notifications sent
// before the response we're waiting for -- see the Streamable HTTP spec's
// "Sending Messages to the Server" section). Returns std::nullopt if the
// body has no "data:" field at all.
std::optional<std::string> extract_last_sse_event_data(const std::string& body);

struct HttpClientTransportConfig {
    std::string url; // e.g. "http://localhost:3001/mcp"
    std::string protocol_version = "2025-06-18";
};

class HttpClientTransport : public ClientTransport {
public:
    explicit HttpClientTransport(HttpClientTransportConfig config);

    nlohmann::json send_request(const nlohmann::json& request) override;
    void send_notification(const nlohmann::json& notification) override;

private:
    HttpClientTransportConfig config_;
    // Captured from the `Mcp-Session-Id` response header on the first
    // response that carries one (normally the initialize response) and
    // echoed back on every subsequent request, per the spec's session
    // management section. Empty if the server never assigned one (session
    // IDs are optional).
    std::string session_id_;
};

} // namespace langchain::mcp::detail
