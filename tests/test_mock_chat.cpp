#include "langchain/llm/chat_model.hpp"
#include "langchain/providers/mock/mock_chat.hpp"

#include <gtest/gtest.h>

using namespace langchain::core;
using namespace langchain::llm;
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

TEST(MockChat, StreamDeliversWordByWordThenFinalChunk) {
    MockChat model("hello there world");

    std::vector<StreamChunk> chunks;
    model.stream({Message::user("hi")}, [&](const StreamChunk& chunk) { chunks.push_back(chunk); });

    ASSERT_EQ(chunks.size(), 4u); // 3 words + final
    EXPECT_EQ(chunks[0].delta, "hello");
    EXPECT_EQ(chunks[1].delta, " there");
    EXPECT_EQ(chunks[2].delta, " world");
    EXPECT_FALSE(chunks[0].is_final);

    EXPECT_TRUE(chunks[3].is_final);
    EXPECT_EQ(chunks[3].delta, "");
    EXPECT_EQ(chunks[3].message.content, "hello there world");
}

TEST(MockChat, StreamOnToolCallMessageDeliversOnlyFinalChunk) {
    MockChat model(std::vector<Message>{
        Message::assistant_tool_calls({ToolCall{"call_1", "lookup", {}}}),
    });

    std::vector<StreamChunk> chunks;
    model.stream({Message::user("hi")}, [&](const StreamChunk& chunk) { chunks.push_back(chunk); });

    ASSERT_EQ(chunks.size(), 1u);
    EXPECT_TRUE(chunks[0].is_final);
    EXPECT_TRUE(chunks[0].message.has_tool_calls());
}

TEST(ChatModel, DefaultStreamSynthesizesSingleFinalChunkFromInvoke) {
    // AnthropicChat/GeminiChat don't override stream() yet, so they go
    // through ChatModel's default. Pinned down here via a minimal
    // ChatModel subclass rather than an existing provider, since MockChat
    // itself overrides stream().
    class NoStreamOverrideModel : public ChatModel {
    public:
        Message invoke(const std::vector<Message>&) override { return Message::assistant("whole reply"); }
        std::string model_name() const override { return "no-stream-override"; }
    };

    NoStreamOverrideModel model;
    std::vector<StreamChunk> chunks;
    model.stream({Message::user("hi")}, [&](const StreamChunk& chunk) { chunks.push_back(chunk); });

    ASSERT_EQ(chunks.size(), 1u);
    EXPECT_TRUE(chunks[0].is_final);
    EXPECT_EQ(chunks[0].message.content, "whole reply");
}
