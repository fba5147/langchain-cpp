#pragma once

#include <nlohmann/json.hpp>

#include <memory>
#include <string>
#include <vector>

namespace langchain::mcp {

namespace detail {
class StdioTransport;
} // namespace detail

struct McpToolInfo {
    std::string name;
    std::string description;
    nlohmann::json input_schema;
};

// A client for a single MCP (Model Context Protocol) server, spawned as a
// subprocess and talked to over its stdin/stdout (the "stdio" transport --
// the common case for local MCP servers like filesystem/git/etc.). Only
// the tools capability is implemented: initialize handshake, tools/list,
// tools/call. Resources, prompts, and sampling aren't -- out of scope for
// wiring MCP server tools into AgentExecutor, which is all ToolRegistry
// needs. See mcp::as_tools() to turn a connected client's tools into
// langchain::tools::Tool instances.
class McpClient {
public:
    // `command` is argv: command[0] is the server executable (resolved via
    // PATH, like execvp), the rest are its arguments -- e.g. {"npx", "-y",
    // "@modelcontextprotocol/server-everything"}. Throws std::runtime_error
    // if the process can't be spawned.
    explicit McpClient(std::vector<std::string> command);
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

    std::unique_ptr<detail::StdioTransport> transport_;
    int next_id_ = 1;
};

} // namespace langchain::mcp
