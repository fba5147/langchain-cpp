// Unit tests for Gemini's request/response conversion. There's no
// GOOGLE_API_KEY available in this environment to smoke-test against the
// real endpoint the way OpenAIChat was verified against Ollama (see
// examples/ollama_demo.cpp) -- testing the pure conversion logic here is
// the next best thing for catching wire-format bugs before they ship.

#include "providers/google/gemini_wire_format.hpp"

#include <gtest/gtest.h>

using namespace langchain::core;
using namespace langchain::providers::detail;

TEST(MessagesToGeminiRequest, MapsUserAndAssistantRolesToUserAndModel) {
    auto request = messages_to_gemini_request({
        Message::user("hi"),
        Message::assistant("hello"),
    });

    ASSERT_EQ(request.contents.size(), 2u);
    EXPECT_EQ(request.contents[0]["role"], "user");
    EXPECT_EQ(request.contents[0]["parts"][0]["text"], "hi");
    EXPECT_EQ(request.contents[1]["role"], "model");
    EXPECT_EQ(request.contents[1]["parts"][0]["text"], "hello");
}

TEST(MessagesToGeminiRequest, ExtractsSystemMessageSeparately) {
    auto request = messages_to_gemini_request({
        Message::system("be nice"),
        Message::user("hi"),
    });

    EXPECT_EQ(request.system_instruction, "be nice");
    ASSERT_EQ(request.contents.size(), 1u);
    EXPECT_EQ(request.contents[0]["role"], "user");
}

TEST(MessagesToGeminiRequest, AssistantToolCallBecomesFunctionCallPart) {
    auto request = messages_to_gemini_request({
        Message::assistant_tool_calls({ToolCall{"call_1", "calculator", {{"a", 1}, {"b", 2}}}}),
    });

    ASSERT_EQ(request.contents.size(), 1u);
    EXPECT_EQ(request.contents[0]["role"], "model");
    auto function_call = request.contents[0]["parts"][0]["functionCall"];
    EXPECT_EQ(function_call["name"], "calculator");
    EXPECT_EQ(function_call["args"]["a"], 1);
}

TEST(MessagesToGeminiRequest, ToolResultBecomesFunctionResponseWithRecoveredName) {
    auto request = messages_to_gemini_request({
        Message::assistant_tool_calls({ToolCall{"call_1", "calculator", {{"a", 1}, {"b", 2}}}}),
        Message::tool_result("call_1", "3"),
    });

    ASSERT_EQ(request.contents.size(), 2u);
    EXPECT_EQ(request.contents[1]["role"], "function");
    auto function_response = request.contents[1]["parts"][0]["functionResponse"];
    // The name is recovered from the preceding tool-call message, since
    // Gemini's protocol has no id to correlate on directly.
    EXPECT_EQ(function_response["name"], "calculator");
    EXPECT_EQ(function_response["response"]["result"], 3);
}

TEST(MessagesToGeminiRequest, ThrowsWhenAnyMessageHasImages) {
    auto message = Message::user_with_images("what's this?", {ImageContent::from_url("https://example.com/x.png")});
    EXPECT_THROW(messages_to_gemini_request({message}), std::runtime_error);
}

TEST(ParseGeminiMessage, PlainTextResponseBecomesAssistantMessage) {
    nlohmann::json response = {
        {"candidates", {{{"content", {{"parts", {{{"text", "hi there"}}}}}}}}},
    };

    Message message = parse_gemini_message(response);

    EXPECT_EQ(message.role, MessageRole::Assistant);
    EXPECT_EQ(message.content, "hi there");
    EXPECT_FALSE(message.has_tool_calls());
}

TEST(ParseGeminiMessage, FunctionCallResponseBecomesToolCallMessage) {
    nlohmann::json response = {
        {"candidates",
         {{{"content",
            {{"parts", {{{"functionCall", {{"name", "calculator"}, {"args", {{"a", 1}, {"b", 2}}}}}}}}}}}}},
    };

    Message message = parse_gemini_message(response);

    ASSERT_TRUE(message.has_tool_calls());
    ASSERT_EQ(message.tool_calls.size(), 1u);
    EXPECT_EQ(message.tool_calls[0].tool_name, "calculator");
    EXPECT_EQ(message.tool_calls[0].arguments["a"], 1);
    // Gemini has no call ids; the id is synthesized, but must still be
    // non-empty so tool_result correlation works.
    EXPECT_FALSE(message.tool_calls[0].id.empty());
}

TEST(ParseGeminiMessage, ThrowsWhenNoCandidates) {
    nlohmann::json response = {{"candidates", nlohmann::json::array()}};
    EXPECT_THROW(parse_gemini_message(response), std::runtime_error);
}
