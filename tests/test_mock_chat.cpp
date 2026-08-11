#include "langchain/providers/mock/mock_chat.hpp"

#include <gtest/gtest.h>

using namespace langchain::core;
using namespace langchain::providers;

TEST(MockChat, ReturnsFixedResponse) {
    MockChat model("fixed reply");
    auto result = model.invoke({Message::user("hello")});
    EXPECT_EQ(result.role, MessageRole::Assistant);
    EXPECT_EQ(result.content, "fixed reply");
}

TEST(MockChat, UsesCustomResponseFunction) {
    MockChat model([](const std::vector<Message>& messages) { return std::to_string(messages.size()) + " messages"; });
    auto result = model.invoke({Message::system("sys"), Message::user("hi")});
    EXPECT_EQ(result.content, "2 messages");
}
