// Direct unit tests for detail::message_to_openai_json/parse_openai_message
// -- previously only exercised indirectly (via a real Ollama call in
// examples/ollama_demo.cpp, or the streaming parser's own tests). Adding
// image support is a good opportunity to backfill direct coverage of the
// base shapes too, alongside the new multi-modal ones.

#include "providers/openai/openai_wire_format.hpp"

#include <gtest/gtest.h>

using namespace langchain::core;
using namespace langchain::providers::detail;

TEST(MessageToOpenaiJson, PlainTextMessageHasStringContent) {
    auto json = message_to_openai_json(Message::user("hello"));
    EXPECT_EQ(json["role"], "user");
    EXPECT_EQ(json["content"], "hello");
}

TEST(MessageToOpenaiJson, ToolResultMessageIncludesToolCallId) {
    auto json = message_to_openai_json(Message::tool_result("call_1", "42"));
    EXPECT_EQ(json["role"], "tool");
    EXPECT_EQ(json["tool_call_id"], "call_1");
    EXPECT_EQ(json["content"], "42");
}

TEST(MessageToOpenaiJson, AssistantToolCallMessageEncodesArgumentsAsJsonString) {
    auto json = message_to_openai_json(
        Message::assistant_tool_calls({ToolCall{"call_1", "calculator", {{"a", 1}, {"b", 2}}}}));
    ASSERT_EQ(json["tool_calls"].size(), 1u);
    EXPECT_EQ(json["tool_calls"][0]["function"]["name"], "calculator");
    // OpenAI expects `arguments` as a JSON-encoded *string*, not a nested object.
    EXPECT_TRUE(json["tool_calls"][0]["function"]["arguments"].is_string());
}

TEST(MessageToOpenaiJson, ImageMessageBecomesContentPartsArray) {
    auto message =
        Message::user_with_images("what's in this?", {ImageContent::from_url("https://example.com/cat.png")});

    auto json = message_to_openai_json(message);

    ASSERT_TRUE(json["content"].is_array());
    ASSERT_EQ(json["content"].size(), 2u);
    EXPECT_EQ(json["content"][0]["type"], "text");
    EXPECT_EQ(json["content"][0]["text"], "what's in this?");
    EXPECT_EQ(json["content"][1]["type"], "image_url");
    EXPECT_EQ(json["content"][1]["image_url"]["url"], "https://example.com/cat.png");
}

TEST(MessageToOpenaiJson, Base64ImageBecomesDataUrl) {
    ImageContent image{ImageSourceType::Base64, "QUJD", "image/png"}; // "QUJD" base64-decodes to "ABC"
    auto message = Message::user_with_images("", {image});

    auto json = message_to_openai_json(message);

    EXPECT_EQ(json["content"][0]["type"], "image_url");
    EXPECT_EQ(json["content"][0]["image_url"]["url"], "data:image/png;base64,QUJD");
}

TEST(MessageToOpenaiJson, ImageMessageWithNoTextOmitsTextPart) {
    auto message = Message::user_with_images("", {ImageContent::from_url("https://example.com/cat.png")});
    auto json = message_to_openai_json(message);

    ASSERT_EQ(json["content"].size(), 1u);
    EXPECT_EQ(json["content"][0]["type"], "image_url");
}

TEST(ParseOpenaiMessage, PlainContentBecomesAssistantMessage) {
    auto message = parse_openai_message(nlohmann::json{{"content", "hi there"}});
    EXPECT_EQ(message.role, MessageRole::Assistant);
    EXPECT_EQ(message.content, "hi there");
}

TEST(ParseOpenaiMessage, ToolCallsBecomeToolCallMessage) {
    nlohmann::json response{
        {"content", nullptr},
        {"tool_calls", {{{"id", "call_1"}, {"function", {{"name", "calculator"}, {"arguments", R"({"a":1})"}}}}}},
    };

    auto message = parse_openai_message(response);

    ASSERT_TRUE(message.has_tool_calls());
    EXPECT_EQ(message.tool_calls[0].id, "call_1");
    EXPECT_EQ(message.tool_calls[0].tool_name, "calculator");
    EXPECT_EQ(message.tool_calls[0].arguments["a"], 1);
}
