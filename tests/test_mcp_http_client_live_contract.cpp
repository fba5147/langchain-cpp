// Same reasoning as test_gemini_chat_live_contract.cpp: this exercises
// McpClient's Streamable HTTP transport (http_client_transport.hpp)
// against a local mock server implementing the documented contract --
// separately cross-checked against a *real* reference server (`npx
// @modelcontextprotocol/server-everything streamableHttp`) while writing
// this, which is where the exact request/response shapes below (Accept
// header, `Mcp-Session-Id` casing, SSE framing with an `event: message`
// line) came from, not just the spec text. This file proves this client
// implements that contract correctly; doesn't prove every MCP server does.

#include "langchain/mcp/mcp_client.hpp"

#include "support/mock_http_server.hpp"

#include <gtest/gtest.h>
#include <nlohmann/json.hpp>

using namespace langchain::mcp;
using namespace langchain::testing;
using json = nlohmann::json;

TEST(McpHttpClientLiveContract, FullToolCallFlowAcrossSseAndJsonResponses) {
    std::vector<MockHttpRequest> captured;
    std::string session_id = "test-session-1234";

    MockHttpServer server([&](const MockHttpRequest& request) {
        captured.push_back(request);
        json body = json::parse(request.body);
        std::string method = body.value("method", std::string());

        if (method == "initialize") {
            json response{{"jsonrpc", "2.0"},
                          {"id", body["id"]},
                          {"result", {{"protocolVersion", "2025-06-18"}, {"capabilities", json::object()}}}};
            return MockHttpResponse{200, "event: message\ndata: " + response.dump() + "\n\n", "text/event-stream"};
        }
        if (method == "notifications/initialized") {
            return MockHttpResponse{202, "", "text/event-stream"};
        }
        if (method == "tools/list") {
            json response{{"jsonrpc", "2.0"},
                          {"id", body["id"]},
                          {"result",
                           {{"tools",
                             {{{"name", "echo"}, {"description", "Echoes input"}, {"inputSchema", json::object()}}}}}}};
            return MockHttpResponse{200, "event: message\ndata: " + response.dump() + "\n\n", "text/event-stream"};
        }
        if (method == "tools/call") {
            json response{{"jsonrpc", "2.0"},
                          {"id", body["id"]},
                          {"result", {{"content", {{{"type", "text"}, {"text", "Echo: hi"}}}}}}};
            // Exercises the plain-JSON response path (the client MUST
            // support both, per the spec), unlike the two calls above.
            return MockHttpResponse{200, response.dump(), "application/json"};
        }
        return MockHttpResponse{404, "{}"};
    });

    McpClient client(McpHttpConfig{server.base_url() + "/mcp"});
    client.initialize();
    auto tools = client.list_tools();
    json result = client.call_tool("echo", json{{"message", "hi"}});

    ASSERT_EQ(tools.size(), 1u);
    EXPECT_EQ(tools[0].name, "echo");
    EXPECT_EQ(result, "Echo: hi");

    ASSERT_EQ(captured.size(), 4u);
    for (const auto& request : captured) {
        EXPECT_EQ(request.method, "POST");
        EXPECT_EQ(request.path, "/mcp");
        EXPECT_EQ(request.header("Accept"), "application/json, text/event-stream");
        EXPECT_EQ(request.header("MCP-Protocol-Version"), "2025-06-18");
    }
    // The initialize request itself has no session yet; every request
    // after it must carry the session ID the mock "assigned" -- wait, the
    // mock above never actually issues Mcp-Session-Id, so assert instead
    // that none of the requests carry one (nothing to echo back).
    for (const auto& request : captured) {
        EXPECT_EQ(request.header("Mcp-Session-Id"), "");
    }
}

TEST(McpHttpClientLiveContract, CapturesAndEchoesBackTheSessionIdFromInitialize) {
    std::vector<std::string> session_ids_sent;
    const std::string assigned_session = "0fbbcc85-04af-49ce-9c2d-aa8f5c9c7db3";

    MockHttpServer server([&](const MockHttpRequest& request) {
        session_ids_sent.push_back(request.header("Mcp-Session-Id"));
        json body = json::parse(request.body);
        std::string method = body.value("method", std::string());

        if (method == "initialize") {
            json response{{"jsonrpc", "2.0"}, {"id", body["id"]}, {"result", json::object()}};
            return MockHttpResponse{200, "event: message\ndata: " + response.dump() + "\n\n", "text/event-stream",
                                     {{"Mcp-Session-Id", assigned_session}}};
        }
        return MockHttpResponse{202, "", "text/event-stream"};
    });

    McpClient client(McpHttpConfig{server.base_url() + "/mcp"});
    client.initialize();

    // The initialize *request* itself carries no session (none assigned
    // yet); the notifications/initialized notification sent right after
    // must carry the one the initialize response just handed back.
    ASSERT_EQ(session_ids_sent.size(), 2u);
    EXPECT_EQ(session_ids_sent[0], "");
    EXPECT_EQ(session_ids_sent[1], assigned_session);
}

TEST(McpHttpClientLiveContract, NonSuccessStatusThrowsWithTheServersOwnErrorBody) {
    MockHttpServer server(
        [](const MockHttpRequest&) { return MockHttpResponse{500, "internal server error", "text/plain"}; });

    McpClient client(McpHttpConfig{server.base_url() + "/mcp"});

    EXPECT_THROW(
        {
            try {
                client.initialize();
            } catch (const std::runtime_error& error) {
                EXPECT_NE(std::string(error.what()).find("internal server error"), std::string::npos);
                throw;
            }
        },
        std::runtime_error);
}

TEST(McpHttpClientLiveContract, JsonRpcErrorInSseResponseThrowsWithServersMessage) {
    MockHttpServer server([](const MockHttpRequest& request) {
        json body = json::parse(request.body);
        json response{{"jsonrpc", "2.0"}, {"id", body["id"]}, {"error", {{"code", -32601}, {"message", "Tool not found"}}}};
        return MockHttpResponse{200, "event: message\ndata: " + response.dump() + "\n\n", "text/event-stream"};
    });

    McpClient client(McpHttpConfig{server.base_url() + "/mcp"});

    EXPECT_THROW(
        {
            try {
                client.list_tools();
            } catch (const std::runtime_error& error) {
                EXPECT_NE(std::string(error.what()).find("Tool not found"), std::string::npos);
                throw;
            }
        },
        std::runtime_error);
}
