#pragma once

// MCP-specific message shapes, layered on top of the generic JSON-RPC
// framing in jsonrpc.hpp. Kept as pure functions so the conversion logic
// is directly unit-testable without spawning a real MCP server (same
// reasoning as the provider wire-format modules).

#include "langchain/core/result.hpp"
#include "langchain/mcp/mcp_client.hpp"
#include "langchain/tools/tool.hpp"

#include <nlohmann/json.hpp>

#include <memory>
#include <vector>

namespace langchain::mcp::detail {

// Client side: params for the initialize request.
nlohmann::json build_initialize_params();

std::vector<McpToolInfo> parse_tools_list_result(const nlohmann::json& result);

// Extracts a CallToolResult's content into a single JSON value: a lone
// text content block becomes a JSON string, anything else (multiple
// blocks, or a non-text block) becomes the raw content array. Throws
// std::runtime_error (with the tool's own error text where available) if
// the result's "isError" is true.
nlohmann::json parse_call_tool_result(const nlohmann::json& result);

// Server side: the result for an initialize request.
nlohmann::json build_initialize_result();

nlohmann::json build_tools_list_result(const std::vector<std::shared_ptr<tools::Tool>>& tools);

// Encodes a tool's Result<json> as a CallToolResult: a string value
// becomes the text verbatim, anything else is JSON-dumped into the text
// block (mirroring parse_call_tool_result's inverse), and an error becomes
// {"isError": true, "content": [{"type": "text", "text": <error message>}]}.
nlohmann::json build_call_tool_result(const core::Result<nlohmann::json>& result);

} // namespace langchain::mcp::detail
