// Demonstrates mcp::McpClient talking to a real MCP server over the
// "Streamable HTTP" transport (mcp::McpHttpConfig) instead of stdio -- see
// mcp_client_demo.cpp for the stdio version, which spawns its server as a
// subprocess itself. A remote/HTTP server is a separate long-lived process
// this client merely connects to, so unlike that demo, you need to start
// one yourself first, in another terminal:
//
//   npx -y @modelcontextprotocol/server-everything streamableHttp
//
// which listens on http://localhost:3001/mcp by default. Verified against
// that exact reference server while building this transport.

#include "langchain/langchain.hpp"

#include <iostream>

using namespace langchain;

int main() {
    std::cout << "--- connecting to the MCP reference server over Streamable HTTP ---\n";
    mcp::McpClient client(mcp::McpHttpConfig{"http://localhost:3001/mcp"});
    client.initialize();

    auto tool_infos = client.list_tools();
    std::cout << "discovered " << tool_infos.size() << " tool(s):\n";
    for (const auto& info : tool_infos) {
        std::cout << "  - " << info.name << ": " << info.description << "\n";
    }
    std::cout << "\n";

    std::cout << "--- calling a tool through McpClient ---\n";
    std::cout << "echo(\"hello over http\") -> " << client.call_tool("echo", {{"message", "hello over http"}}).dump()
              << "\n";

    return 0;
}
