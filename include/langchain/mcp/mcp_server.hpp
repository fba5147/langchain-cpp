#pragma once

#include "langchain/tools/tool_registry.hpp"

#include <nlohmann/json.hpp>

#include <iostream>
#include <memory>
#include <optional>

namespace langchain::mcp {

// Exposes a ToolRegistry's tools as an MCP server. Only the tools
// capability is implemented: initialize, tools/list, tools/call. Two
// transports share the same request handling (handle_message()): stdio
// (serve(), see examples/mcp_server_demo.cpp) and Streamable HTTP
// (McpHttpServer, mcp_http_server.hpp).
class McpServer {
public:
    explicit McpServer(std::shared_ptr<tools::ToolRegistry> registry);

    // Blocks, handling one request per line, until `in` reaches EOF (the
    // client closed the connection). Meant to be run as an entire program
    // that an external MCP client -- Claude Desktop, another langchain-cpp
    // process's McpClient, etc. -- spawns as a subprocess and talks to over
    // its stdin/stdout.
    void serve(std::istream& in = std::cin, std::ostream& out = std::cout);

    // Dispatches a single already-parsed JSON-RPC message. Returns the
    // response for a request; std::nullopt for a notification (or a stray
    // response), which needs none.
    std::optional<nlohmann::json> handle_message(const nlohmann::json& message);

private:
    std::shared_ptr<tools::ToolRegistry> registry_;
};

} // namespace langchain::mcp
