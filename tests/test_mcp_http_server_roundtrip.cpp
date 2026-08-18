// Same idea as test_mcp_server_roundtrip.cpp, but for the Streamable HTTP
// transport on both ends: mcp::McpHttpServer serves a "calculator" tool
// registry on a background thread bound to an OS-assigned port, and
// mcp::McpClient (constructed with McpHttpConfig) talks to it over real
// HTTP -- both sides are this project's own code, so this is an automated,
// CI-safe (no network beyond loopback) regression test proving they
// actually interoperate, complementing the manual verification against the
// real `npx @modelcontextprotocol/server-everything streamableHttp`
// reference server (see CONTRIBUTING.md).

#include "langchain/mcp/mcp_client.hpp"
#include "langchain/mcp/mcp_http_server.hpp"
#include "langchain/mcp/mcp_tools.hpp"
#include "langchain/tools/function_tool.hpp"
#include "langchain/tools/tool_registry.hpp"

#include <cpr/cpr.h>
#include <gtest/gtest.h>

using namespace langchain::mcp;
using namespace langchain::tools;

namespace {

std::shared_ptr<ToolRegistry> calculator_registry() {
    auto registry = std::make_shared<ToolRegistry>();
    registry->add(std::make_shared<FunctionTool>(
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
        nlohmann::json{{"type", "object"}, {"required", {"a", "b", "op"}}}));
    return registry;
}

std::shared_ptr<McpClient> connect(const McpHttpServer& server) {
    auto client = std::make_shared<McpClient>(McpHttpConfig{server.url()});
    client->initialize();
    return client;
}

} // namespace

TEST(McpHttpServerRoundtrip, ListToolsDiscoversTheCalculatorTool) {
    McpHttpServer server(calculator_registry());
    auto client = connect(server);

    auto tool_infos = client->list_tools();

    ASSERT_EQ(tool_infos.size(), 1u);
    EXPECT_EQ(tool_infos[0].name, "calculator");
    EXPECT_EQ(tool_infos[0].input_schema["type"], "object");
}

TEST(McpHttpServerRoundtrip, CallToolReturnsTheComputedResult) {
    McpHttpServer server(calculator_registry());
    auto client = connect(server);

    auto result = client->call_tool("calculator", {{"a", 6}, {"b", 7}, {"op", "*"}});

    ASSERT_TRUE(result.is_string());
    EXPECT_EQ(result.get<std::string>(), "42.0");
}

TEST(McpHttpServerRoundtrip, CallingAnUnknownToolThrows) {
    McpHttpServer server(calculator_registry());
    auto client = connect(server);

    EXPECT_THROW(client->call_tool("no_such_tool", {}), std::runtime_error);
}

TEST(McpHttpServerRoundtrip, AsToolsWrapsTheServersToolForAnAgent) {
    McpHttpServer server(calculator_registry());
    auto client = connect(server);

    auto wrapped = as_tools(client);

    ASSERT_EQ(wrapped.size(), 1u);
    EXPECT_EQ(wrapped[0]->name(), "calculator");

    auto result = wrapped[0]->call({{"a", 10}, {"b", 4}, {"op", "-"}});
    ASSERT_TRUE(result.ok());
    EXPECT_EQ(result.value().get<std::string>(), "6.0");
}

TEST(McpHttpServerRoundtrip, GetRequestToTheMcpEndpointIsRejected) {
    McpHttpServer server(calculator_registry());

    // No cpr::Get in this test binary's usual dependency set beyond what
    // McpClient already needs -- reuse it directly to hit the endpoint with
    // the one HTTP method this server deliberately doesn't support.
    cpr::Response response = cpr::Get(cpr::Url{server.url()});
    EXPECT_EQ(response.status_code, 405);
}
