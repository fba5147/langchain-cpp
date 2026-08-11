#include "langchain/core/message.hpp"

#include <gtest/gtest.h>

using namespace langchain::core;

TEST(Message, FactoriesSetRoleAndContent) {
    EXPECT_EQ(Message::system("s").role, MessageRole::System);
    EXPECT_EQ(Message::user("u").role, MessageRole::User);
    EXPECT_EQ(Message::assistant("a").role, MessageRole::Assistant);
    EXPECT_EQ(Message::tool_result("call_1", "t").role, MessageRole::Tool);
    EXPECT_EQ(Message::user("hello").content, "hello");
}

TEST(Message, ToApiRoleMapsToWireStrings) {
    EXPECT_EQ(to_api_role(MessageRole::System), "system");
    EXPECT_EQ(to_api_role(MessageRole::User), "user");
    EXPECT_EQ(to_api_role(MessageRole::Assistant), "assistant");
    EXPECT_EQ(to_api_role(MessageRole::Tool), "tool");
}

TEST(Message, AssistantToolCallsCarriesCallsAndOptionalContent) {
    Message message = Message::assistant_tool_calls({ToolCall{"call_1", "add", {{"a", 1}, {"b", 2}}}}, "thinking...");
    EXPECT_EQ(message.role, MessageRole::Assistant);
    EXPECT_TRUE(message.has_tool_calls());
    ASSERT_EQ(message.tool_calls.size(), 1u);
    EXPECT_EQ(message.tool_calls[0].id, "call_1");
    EXPECT_EQ(message.tool_calls[0].tool_name, "add");
    EXPECT_EQ(message.tool_calls[0].arguments["a"], 1);
    EXPECT_EQ(message.content, "thinking...");
}

TEST(Message, ToolResultCarriesCallId) {
    Message message = Message::tool_result("call_1", "3");
    EXPECT_EQ(message.role, MessageRole::Tool);
    EXPECT_EQ(message.tool_call_id, "call_1");
    EXPECT_EQ(message.content, "3");
    EXPECT_FALSE(message.has_tool_calls());
}
