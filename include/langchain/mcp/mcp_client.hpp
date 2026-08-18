#pragma once

#include <nlohmann/json.hpp>

#include <memory>
#include <string>
#include <vector>

namespace langchain::mcp {

namespace detail {
class ClientTransport;
} // namespace detail

struct McpToolInfo {
    std::string name;
    std::string description;
    nlohmann::json input_schema;
};

// Connects to a remote MCP server over its "Streamable HTTP" transport
// (the transport that replaced the older separate HTTP+SSE transport in the
// 2025-03-26 spec revision) instead of spawning a local subprocess. `url` is
// the server's single MCP endpoint, e.g. "http://localhost:3001/mcp".
struct McpHttpConfig {
    std::string url;
};

// A client for a single MCP (Model Context Protocol) server. Only the tools
// capability is implemented: initialize handshake, tools/list, tools/call.
// Resources, prompts, and sampling aren't -- out of scope for wiring MCP
// server tools into AgentExecutor, which is all ToolRegistry needs. See
// mcp::as_tools() to turn a connected client's tools into
// langchain::tools::Tool instances.
class McpClient {
public:
    // Spawns the server as a subprocess and talks to it over its
    // stdin/stdout (the "stdio" transport -- the common case for local MCP
    // servers like filesystem/git/etc.). `command` is argv: command[0] is
    // the server executable (resolved via PATH, like execvp), the rest are
    // its arguments -- e.g. {"npx", "-y", "@modelcontextprotocol/server-everything"}.
    // Throws std::runtime_error if the process can't be spawned.
    explicit McpClient(std::vector<std::string> command);
    // Connects to a remote server over Streamable HTTP instead -- see
    // McpHttpConfig.
    explicit McpClient(McpHttpConfig config);
    ~McpClient();

    McpClient(const McpClient&) = delete;
    McpClient& operator=(const McpClient&) = delete;

    // Performs the initialize handshake (initialize request +
    // notifications/initialized). Must be called once before
    // list_tools()/call_tool(). Throws std::runtime_error on a malformed
    // or missing response.
    void initialize();

    std::vector<McpToolInfo> list_tools();

    // Throws std::runtime_error if the server reports isError, or if the
    // response is malformed.
    nlohmann::json call_tool(const std::string& name, const nlohmann::json& arguments);

private:
    nlohmann::json send_request(const std::string& method, const nlohmann::json& params);
    void send_notification(const std::string& method, const nlohmann::json& params);

    std::unique_ptr<detail::ClientTransport> transport_;
    int next_id_ = 1;
};

} // namespace langchain::mcp
