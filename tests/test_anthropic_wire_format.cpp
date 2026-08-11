// Direct unit tests for detail::message_to_anthropic_json/parse_anthropic_message
// -- extracted from anthropic_chat.cpp (Part H) specifically so these
// become testable without a live ANTHROPIC_API_KEY, backfilling coverage
// of the base shapes alongside the new image-rejection behavior.

#include "providers/anthropic/anthropic_wire_format.hpp"

#include <gtest/gtest.h>

using namespace langchain::core;
using namespace langchain::providers::detail;

TEST(MessageToAnthropicJson, PlainTextMessageHasStringContent) {
    auto json = message_to_anthropic_json(Message::user("hello"));
    EXPECT_EQ(json["role"], "user");
    EXPECT_EQ(json["content"], "hello");
}

TEST(MessageToAnthropicJson, ToolResultBecomesUserRoleWithToolResultBlock) {
    // Anthropic has no "tool" role -- our Tool-role message must map to
    // a "user" message containing a tool_result content block.
    auto json = message_to_anthropic_json(Message::tool_result("call_1", "42"));

    EXPECT_EQ(json["role"], "user");
    ASSERT_EQ(json["content"].size(), 1u);
    EXPECT_EQ(json["content"][0]["type"], "tool_result");
    EXPECT_EQ(json["content"][0]["tool_use_id"], "call_1");
    EXPECT_EQ(json["content"][0]["content"], "42");
}

TEST(MessageToAnthropicJson, AssistantToolCallBecomesToolUseBlock) {
    auto message = Message::assistant_tool_calls({ToolCall{"call_1", "calculator", {{"a", 1}}}}, "thinking");
    auto json = message_to_anthropic_json(message);

    EXPECT_EQ(json["role"], "assistant");
    ASSERT_EQ(json["content"].size(), 2u);
    EXPECT_EQ(json["content"][0]["type"], "text");
    EXPECT_EQ(json["content"][0]["text"], "thinking");
    EXPECT_EQ(json["content"][1]["type"], "tool_use");
    EXPECT_EQ(json["content"][1]["id"], "call_1");
    EXPECT_EQ(json["content"][1]["name"], "calculator");
    EXPECT_EQ(json["content"][1]["input"]["a"], 1);
}

TEST(MessageToAnthropicJson, ThrowsWhenMessageHasImages) {
    auto message = Message::user_with_images("what's this?", {ImageContent::from_url("https://example.com/x.png")});
    EXPECT_THROW(message_to_anthropic_json(message), std::runtime_error);
}

TEST(ParseAnthropicMessage, TextBlockBecomesAssistantMessage) {
    nlohmann::json response{{"content", {{{"type", "text"}, {"text", "hi there"}}}}};
    auto message = parse_anthropic_message(response);
    EXPECT_EQ(message.role, MessageRole::Assistant);
    EXPECT_EQ(message.content, "hi there");
}

TEST(ParseAnthropicMessage, ToolUseBlockBecomesToolCallMessage) {
    nlohmann::json response{
        {"content", {{{"type", "tool_use"}, {"id", "call_1"}, {"name", "calculator"}, {"input", {{"a", 1}}}}}}};

    auto message = parse_anthropic_message(response);

    ASSERT_TRUE(message.has_tool_calls());
    EXPECT_EQ(message.tool_calls[0].id, "call_1");
    EXPECT_EQ(message.tool_calls[0].tool_name, "calculator");
    EXPECT_EQ(message.tool_calls[0].arguments["a"], 1);
}
