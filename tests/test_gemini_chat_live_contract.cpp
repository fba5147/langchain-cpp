// Same reasoning as test_anthropic_chat_live_contract.cpp: GeminiChat's
// pure conversion logic is already unit tested
// (test_gemini_wire_format.cpp) and cross-checked by hand against
// Google's official generateContent docs while adding this file --
// notably confirming `x-goog-api-key` is the *current* recommended
// header (not the legacy `?key=` query parameter, which older guides
// still show). This file exercises the HTTP layer itself against a local
// mock server speaking that documented contract, since no GOOGLE_API_KEY
// is available in this environment. Proves this client implements the
// contract correctly; doesn't prove Google's actual server does.

#include "langchain/providers/google/gemini_chat.hpp"
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

GeminiConfig test_config(const std::string& base_url) {
    GeminiConfig config;
    config.api_key = "test-gemini-key";
    config.base_url = base_url + "/v1beta";
    return config;
}

} // namespace

TEST(GeminiChatLiveContract, PlainTextRequestHitsDocumentedUrlWithDocumentedHeader) {
    MockHttpRequest captured;
    MockHttpServer server([&](const MockHttpRequest& request) {
        captured = request;
        return MockHttpResponse{
            200, json{{"candidates", {{{"content", {{"role", "model"}, {"parts", {{{"text", "Hello there!"}}}}}}}}}}
                     .dump()};
    });

    GeminiChat chat(test_config(server.base_url()));
    Message reply = chat.invoke({Message::user("hi")});

    EXPECT_EQ(reply.content, "Hello there!");

    EXPECT_EQ(captured.method, "POST");
    EXPECT_EQ(captured.path, "/v1beta/models/gemini-2.5-flash:generateContent");
    EXPECT_EQ(captured.header("x-goog-api-key"), "test-gemini-key");
    EXPECT_EQ(captured.header("Content-Type"), "application/json");

    json body = json::parse(captured.body);
    EXPECT_EQ(body["contents"][0]["role"], "user");
    EXPECT_EQ(body["contents"][0]["parts"][0]["text"], "hi");
    EXPECT_FALSE(body.contains("systemInstruction"));
    EXPECT_FALSE(body.contains("tools"));
}

TEST(GeminiChatLiveContract, SystemInstructionAndBoundToolsAppearInRequestBody) {
    MockHttpRequest captured;
    MockHttpServer server([&](const MockHttpRequest& request) {
        captured = request;
        return MockHttpResponse{
            200, json{{"candidates", {{{"content", {{"role", "model"}, {"parts", {{{"text", "ok"}}}}}}}}}}.dump()};
    });

    GeminiChat chat(test_config(server.base_url()));
    auto registry = std::make_shared<langchain::tools::ToolRegistry>();
    registry->add(std::make_shared<langchain::tools::FunctionTool>(
        "get_weather", "Looks up the weather", [](const json&) -> json { return "sunny"; },
        json{{"type", "object"}}));

    auto bound = chat.bind_tools(registry);
    bound->invoke({Message::system("be nice"), Message::user("what's the weather?")});

    json body = json::parse(captured.body);
    EXPECT_EQ(body["systemInstruction"]["parts"][0]["text"], "be nice");
    ASSERT_TRUE(body.contains("tools"));
    ASSERT_EQ(body["tools"][0]["functionDeclarations"].size(), 1u);
    EXPECT_EQ(body["tools"][0]["functionDeclarations"][0]["name"], "get_weather");
}

TEST(GeminiChatLiveContract, ParsesFunctionCallResponseIntoToolCallMessage) {
    json function_call{{"name", "get_weather"}, {"args", {{"city", "Paris"}}}};
    json part{{"functionCall", function_call}};
    json content{{"role", "model"}, {"parts", json::array({part})}};
    json candidate{{"content", content}};
    json response_body{{"candidates", json::array({candidate})}};

    MockHttpServer server(
        [&](const MockHttpRequest&) { return MockHttpResponse{200, response_body.dump()}; });

    GeminiChat chat(test_config(server.base_url()));
    Message reply = chat.invoke({Message::user("weather in Paris?")});

    ASSERT_TRUE(reply.has_tool_calls());
    EXPECT_EQ(reply.tool_calls[0].tool_name, "get_weather");
    EXPECT_EQ(reply.tool_calls[0].arguments["city"], "Paris");
}

TEST(GeminiChatLiveContract, NonSuccessStatusThrowsWithTheServersOwnErrorBody) {
    MockHttpServer server([](const MockHttpRequest&) {
        return MockHttpResponse{
            400, json{{"error", {{"code", 400}, {"message", "API key not valid"}, {"status", "INVALID_ARGUMENT"}}}}
                     .dump()};
    });

    GeminiChat chat(test_config(server.base_url()));

    EXPECT_THROW(
        {
            try {
                chat.invoke({Message::user("hi")});
            } catch (const std::runtime_error& error) {
                EXPECT_NE(std::string(error.what()).find("API key not valid"), std::string::npos);
                throw;
            }
        },
        std::runtime_error);
}
