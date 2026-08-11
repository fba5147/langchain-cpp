// AnthropicChat's request/response *conversion logic* is already unit
// tested directly (test_anthropic_wire_format.cpp), and cross-checked by
// hand against the official Messages API docs while adding this file --
// headers (`x-api-key`, `anthropic-version`), URL (`POST /v1/messages`),
// and body/response shapes all matched. What neither of those covers is
// whether AnthropicChat's *HTTP layer* actually sends what it claims:
// correct URL, correct headers, a real request/response round trip
// through cpr. No ANTHROPIC_API_KEY is available in this environment, so
// this spins up a local HTTP server (support/mock_http_server.hpp) that
// speaks the documented contract and points AnthropicChat at it via
// `base_url` -- real sockets, real HTTP, just not the real
// api.anthropic.com. This does NOT prove Anthropic's actual server
// behaves identically; it proves this client faithfully implements the
// documented contract, which is the part actually under this project's
// control.

#include "langchain/providers/anthropic/anthropic_chat.hpp"
#include "langchain/tools/function_tool.hpp"
#include "langchain/tools/tool_registry.hpp"

#include "support/mock_http_server.hpp"

#include <gtest/gtest.h>
#include <nlohmann/json.hpp>

using namespace langchain::core;
using namespace langchain::providers;
using namespace langchain::testing;
using json = nlohmann::json;

namespace {

AnthropicConfig test_config(const std::string& base_url) {
    AnthropicConfig config;
    config.api_key = "test-anthropic-key";
    config.base_url = base_url + "/v1";
    return config;
}

} // namespace

TEST(AnthropicChatLiveContract, PlainTextRequestHitsDocumentedUrlWithDocumentedHeaders) {
    MockHttpRequest captured;
    MockHttpServer server([&](const MockHttpRequest& request) {
        captured = request;
        return MockHttpResponse{200, json{
                                          {"id", "msg_1"},
                                          {"type", "message"},
                                          {"role", "assistant"},
                                          {"model", "claude-sonnet-5"},
                                          {"content", {{{"type", "text"}, {"text", "Hello there!"}}}},
                                          {"stop_reason", "end_turn"},
                                      }
                                      .dump()};
    });

    AnthropicChat chat(test_config(server.base_url()));
    Message reply = chat.invoke({Message::user("hi")});

    EXPECT_EQ(reply.content, "Hello there!");

    EXPECT_EQ(captured.method, "POST");
    EXPECT_EQ(captured.path, "/v1/messages");
    EXPECT_EQ(captured.header("x-api-key"), "test-anthropic-key");
    EXPECT_EQ(captured.header("anthropic-version"), "2023-06-01");
    // Header names are case-insensitive on the wire; capturing via a
    // lowercased lookup (rather than an exact-case string match) is the
    // point -- it proves the *value* arrived correctly regardless of how
    // cpr/libcurl happened to capitalize the name in transit.
    EXPECT_EQ(captured.header("Content-Type"), "application/json");

    json body = json::parse(captured.body);
    EXPECT_EQ(body["model"], "claude-sonnet-5");
    EXPECT_EQ(body["messages"][0]["role"], "user");
    EXPECT_EQ(body["messages"][0]["content"], "hi");
    EXPECT_FALSE(body.contains("system"));
    EXPECT_FALSE(body.contains("tools"));
}

TEST(AnthropicChatLiveContract, SystemMessageAndBoundToolsAppearInRequestBody) {
    MockHttpRequest captured;
    MockHttpServer server([&](const MockHttpRequest& request) {
        captured = request;
        return MockHttpResponse{200, json{{"id", "msg_2"},
                                           {"type", "message"},
                                           {"role", "assistant"},
                                           {"model", "claude-sonnet-5"},
                                           {"content", {{{"type", "text"}, {"text", "ok"}}}},
                                           {"stop_reason", "end_turn"}}
                                      .dump()};
    });

    AnthropicChat chat(test_config(server.base_url()));
    auto registry = std::make_shared<langchain::tools::ToolRegistry>();
    registry->add(std::make_shared<langchain::tools::FunctionTool>(
        "get_weather", "Looks up the weather", [](const json&) -> json { return "sunny"; },
        json{{"type", "object"}}));

    auto bound = chat.bind_tools(registry);
    bound->invoke({Message::system("be nice"), Message::user("what's the weather?")});

    json body = json::parse(captured.body);
    EXPECT_EQ(body["system"], "be nice");
    ASSERT_TRUE(body.contains("tools"));
    ASSERT_EQ(body["tools"].size(), 1u);
    EXPECT_EQ(body["tools"][0]["name"], "get_weather");
    EXPECT_EQ(body["tools"][0]["input_schema"]["type"], "object");
}

TEST(AnthropicChatLiveContract, ParsesToolUseResponseIntoToolCallMessage) {
    MockHttpServer server([](const MockHttpRequest&) {
        return MockHttpResponse{
            200,
            json{
                {"id", "msg_3"},
                {"type", "message"},
                {"role", "assistant"},
                {"model", "claude-sonnet-5"},
                {"content", {{{"type", "tool_use"}, {"id", "toolu_1"}, {"name", "get_weather"}, {"input", {{"city", "Paris"}}}}}},
                {"stop_reason", "tool_use"},
            }
                .dump()};
    });

    AnthropicChat chat(test_config(server.base_url()));
    Message reply = chat.invoke({Message::user("weather in Paris?")});

    ASSERT_TRUE(reply.has_tool_calls());
    EXPECT_EQ(reply.tool_calls[0].id, "toolu_1");
    EXPECT_EQ(reply.tool_calls[0].tool_name, "get_weather");
    EXPECT_EQ(reply.tool_calls[0].arguments["city"], "Paris");
}

TEST(AnthropicChatLiveContract, NonSuccessStatusThrowsWithTheServersOwnErrorBody) {
    MockHttpServer server([](const MockHttpRequest&) {
        return MockHttpResponse{
            401, json{{"type", "error"}, {"error", {{"type", "authentication_error"}, {"message", "invalid x-api-key"}}}}
                     .dump()};
    });

    AnthropicChat chat(test_config(server.base_url()));

    EXPECT_THROW(
        {
            try {
                chat.invoke({Message::user("hi")});
            } catch (const std::runtime_error& error) {
                EXPECT_NE(std::string(error.what()).find("invalid x-api-key"), std::string::npos);
                throw;
            }
        },
        std::runtime_error);
}
