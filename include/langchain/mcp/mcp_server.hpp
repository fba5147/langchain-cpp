#pragma once

#include "langchain/tools/tool_registry.hpp"

#include <iostream>
#include <memory>

namespace langchain::mcp {

// Exposes a ToolRegistry's tools as an MCP server over stdio: reads
// newline-delimited JSON-RPC requests from `in`, dispatches
// initialize/tools/list/tools/call, and writes responses to `out`. Meant
// to be run as an entire program (see examples/mcp_server_demo.cpp) that
// an external MCP client -- Claude Desktop, another langchain-cpp
// process's McpClient, etc. -- spawns as a subprocess and talks to over
// its stdin/stdout. Only the tools capability is implemented, symmetric
// with McpClient.
class McpServer {
public:
    explicit McpServer(std::shared_ptr<tools::ToolRegistry> registry);

    // Blocks, handling one request per line, until `in` reaches EOF (the
    // client closed the connection).
    void serve(std::istream& in = std::cin, std::ostream& out = std::cout);

private:
    std::shared_ptr<tools::ToolRegistry> registry_;
};

} // namespace langchain::mcp
