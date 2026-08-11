// A real MCP server: exposes a single "calculator" tool over stdio.
// Meant to be spawned as a subprocess by an MCP client -- either an
// external one (Claude Desktop, any MCP-speaking agent: point it at this
// binary's path) or langchain-cpp's own McpClient, which is exactly how
// tests/test_mcp_server_roundtrip.cpp verifies this server and
// mcp::McpClient actually interoperate. Not meant to be run directly from
// a terminal and typed at -- it speaks newline-delimited JSON-RPC, not a
// human-friendly REPL.

#include "langchain/langchain.hpp"

using namespace langchain;

int main() {
    auto registry = std::make_shared<tools::ToolRegistry>();
    registry->add(std::make_shared<tools::FunctionTool>(
        "calculator", "Evaluates a simple `a <op> b` arithmetic expression.",
        [](const nlohmann::json& input) -> nlohmann::json {
            double a = input.at("a").get<double>();
            double b = input.at("b").get<double>();
            std::string op = input.at("op").get<std::string>();
            if (op == "+") return a + b;
            if (op == "-") return a - b;
            if (op == "*") return a * b;
            if (op == "/") return a / b;
            throw std::invalid_argument("unsupported op: " + op);
        },
        nlohmann::json{
            {"type", "object"},
            {"properties",
             {
                 {"a", {{"type", "number"}}},
                 {"b", {{"type", "number"}}},
                 {"op", {{"type", "string"}, {"enum", {"+", "-", "*", "/"}}}},
             }},
            {"required", {"a", "b", "op"}},
        }));

    mcp::McpServer server(registry);
    server.serve();

    return 0;
}
