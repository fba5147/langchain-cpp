#include "langchain/core/message.hpp"

#include <gtest/gtest.h>

using namespace langchain::core;

TEST(Message, FactoriesSetRoleAndContent) {
    EXPECT_EQ(Message::system("s").role, MessageRole::System);
    EXPECT_EQ(Message::user("u").role, MessageRole::User);
    EXPECT_EQ(Message::assistant("a").role, MessageRole::Assistant);
    EXPECT_EQ(Message::tool("t").role, MessageRole::Tool);
    EXPECT_EQ(Message::user("hello").content, "hello");
}

TEST(Message, ToApiRoleMapsToWireStrings) {
    EXPECT_EQ(to_api_role(MessageRole::System), "system");
    EXPECT_EQ(to_api_role(MessageRole::User), "user");
    EXPECT_EQ(to_api_role(MessageRole::Assistant), "assistant");
    EXPECT_EQ(to_api_role(MessageRole::Tool), "tool");
}
