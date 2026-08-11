#pragma once

// Generic JSON-RPC 2.0 message framing -- no MCP-specific knowledge here
// (see mcp_protocol.hpp for that). Kept separate and directly testable
// without spawning a process, same reasoning as the provider wire-format
// modules under src/providers/*/.

#include <nlohmann/json.hpp>

#include <string>

namespace langchain::mcp::detail {

nlohmann::json build_request(int id, const std::string& method, const nlohmann::json& params);
nlohmann::json build_notification(const std::string& method, const nlohmann::json& params);

// A response (to a request we sent) has an "id" and no "method"; a
// request or notification *from* the server has a "method" instead --
// this is how the two are told apart on the read side, since both share
// the same stdio stream.
bool is_response(const nlohmann::json& message);

struct JsonRpcResponse {
    nlohmann::json result;
    bool is_error = false;
    std::string error_message;
};

// Expects is_response(message) to be true.
JsonRpcResponse parse_response(const nlohmann::json& message);

// Server-side: a request (as opposed to a notification) has both "method"
// and "id" -- the id must be echoed back verbatim in the response, so it's
// kept as a generic json value rather than assumed to be an int (the spec
// allows string ids too, even though this project's own McpClient only
// ever sends integers).
bool is_request(const nlohmann::json& message);

struct JsonRpcRequest {
    nlohmann::json id;
    std::string method;
    nlohmann::json params;
};

// Expects is_request(message) to be true.
JsonRpcRequest parse_request(const nlohmann::json& message);

nlohmann::json build_success_response(const nlohmann::json& id, const nlohmann::json& result);
nlohmann::json build_error_response(const nlohmann::json& id, int code, const std::string& message);

} // namespace langchain::mcp::detail
