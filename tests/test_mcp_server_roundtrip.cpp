// Verifies langchain-cpp's own McpClient and McpServer actually
// interoperate: spawns the built mcp_server_demo binary (a real MCP
// server exposing a "calculator" tool) as a subprocess and talks to it
// exactly like examples/mcp_client_demo.cpp talks to the official
// reference server -- except both ends are this project's own code here,
// so it's an automated, CI-safe (no network, no npx) regression test for
// the whole client/server round trip. MCP_SERVER_DEMO_PATH is injected by
// CMake as the real path to the built mcp_server_demo binary.

#include "langchain/mcp/mcp_client.hpp"
#include "langchain/mcp/mcp_tools.hpp"

#include <gtest/gtest.h>

using namespace langchain::mcp;

namespace {

std::shared_ptr<McpClient> connect() {
    auto client = std::make_shared<McpClient>(std::vector<std::string>{MCP_SERVER_DEMO_PATH});
    client->initialize();
    return client;
}

} // namespace

TEST(McpServerRoundtrip, ListToolsDiscoversTheCalculatorTool) {
    auto client = connect();
    auto tool_infos = client->list_tools();

    ASSERT_EQ(tool_infos.size(), 1u);
    EXPECT_EQ(tool_infos[0].name, "calculator");
    EXPECT_EQ(tool_infos[0].input_schema["type"], "object");
}

TEST(McpServerRoundtrip, CallToolReturnsTheComputedResult) {
    auto client = connect();
    auto result = client->call_tool("calculator", {{"a", 6}, {"b", 7}, {"op", "*"}});
    ASSERT_TRUE(result.is_string());
    EXPECT_EQ(result.get<std::string>(), "42.0");
}

TEST(McpServerRoundtrip, CallingAnUnknownToolThrows) {
    auto client = connect();
    EXPECT_THROW(client->call_tool("no_such_tool", {}), std::runtime_error);
}

TEST(McpServerRoundtrip, AsToolsWrapsTheServersToolForAnAgent) {
    auto client = connect();
    auto tools = as_tools(client);

    ASSERT_EQ(tools.size(), 1u);
    EXPECT_EQ(tools[0]->name(), "calculator");

    auto result = tools[0]->call({{"a", 10}, {"b", 4}, {"op", "-"}});
    ASSERT_TRUE(result.ok());
    EXPECT_EQ(result.value().get<std::string>(), "6.0");
}
