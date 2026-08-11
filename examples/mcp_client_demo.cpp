// Demonstrates mcp::McpClient talking to a real MCP server over stdio --
// here, the official reference "everything" server, spawned via `npx`
// (Node.js). Lists its tools, calls a couple directly, then wraps them
// with mcp::as_tools() and runs a real AgentExecutor against a local
// Ollama server (llama3.2) to prove the full loop: an LLM picks an MCP
// tool, langchain-cpp calls out to the MCP server over the stdio
// transport, and the result feeds back as a tool_result message -- the
// same path any other Tool goes through.
//
// Requires `npx` (Node.js) on PATH -- the first run downloads the
// reference server package, which takes a few seconds -- and `ollama
// serve` running locally with llama3.2 pulled for the agent section
// (verified against both, see CHANGELOG).

#include "langchain/langchain.hpp"

#include <iostream>

using namespace langchain;

int main() {
    std::cout << "--- connecting to the MCP reference server (via npx) ---\n";
    auto client = std::make_shared<mcp::McpClient>(
        std::vector<std::string>{"npx", "-y", "@modelcontextprotocol/server-everything"});
    client->initialize();

    auto tool_infos = client->list_tools();
    std::cout << "discovered " << tool_infos.size() << " tool(s):\n";
    for (const auto& info : tool_infos) {
        std::cout << "  - " << info.name << ": " << info.description << "\n";
    }
    std::cout << "\n";

    std::cout << "--- calling tools directly through McpClient ---\n";
    std::cout << "get-sum(47, 89) -> " << client->call_tool("get-sum", {{"a", 47}, {"b", 89}}).dump() << "\n";
    std::cout << "echo(\"hello mcp\") -> " << client->call_tool("echo", {{"message", "hello mcp"}}).dump() << "\n\n";

    std::cout << "--- wiring MCP tools into a real agent (via local Ollama) ---\n";
    auto registry = std::make_shared<tools::ToolRegistry>();
    for (const auto& tool : mcp::as_tools(client)) {
        registry->add(tool);
    }

    auto model = std::make_shared<providers::OpenAIChat>(providers::OpenAIConfig{
        .model = "llama3.2",
        .api_key = "ollama", // Ollama doesn't check the key, but our config requires a non-empty one.
        .base_url = "http://localhost:11434/v1",
    });

    agents::AgentExecutor agent(model, registry);
    core::Message answer = agent.run("What is 47 plus 89? Use the get-sum tool to compute it.");
    std::cout << answer.content << "\n";

    return 0;
}
