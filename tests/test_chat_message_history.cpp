#include "langchain/llm/chat_model_with_history.hpp"
#include "langchain/llm/file_chat_message_history.hpp"
#include "langchain/llm/in_memory_chat_message_history.hpp"

#include <gtest/gtest.h>

#include <filesystem>

using namespace langchain::core;
using namespace langchain::llm;

namespace {

// Reports how many messages it was actually given, so growth of the
// conversation across turns is directly observable.
class HistoryAwareModel : public ChatModel {
public:
    Message invoke(const std::vector<Message>& messages) override {
        return Message::assistant("saw " + std::to_string(messages.size()) + " message(s)");
    }
    std::string model_name() const override { return "history-aware-model"; }
};

} // namespace

TEST(InMemoryChatMessageHistory, StartsEmptyAndAccumulatesInOrder) {
    InMemoryChatMessageHistory history;
    EXPECT_TRUE(history.messages().empty());

    history.add_message(Message::user("hi"));
    history.add_message(Message::assistant("hello"));

    auto messages = history.messages();
    ASSERT_EQ(messages.size(), 2u);
    EXPECT_EQ(messages[0].content, "hi");
    EXPECT_EQ(messages[1].content, "hello");
}

TEST(InMemoryChatMessageHistory, ClearEmptiesHistory) {
    InMemoryChatMessageHistory history;
    history.add_message(Message::user("hi"));
    history.clear();
    EXPECT_TRUE(history.messages().empty());
}

TEST(FileChatMessageHistory, MissingFileStartsEmpty) {
    auto path = std::filesystem::temp_directory_path() / "langchain_cpp_missing_history.json";
    std::filesystem::remove(path);

    FileChatMessageHistory history(path.string());
    EXPECT_TRUE(history.messages().empty());
}

TEST(FileChatMessageHistory, PersistsAcrossInstancesIncludingToolCalls) {
    auto path = std::filesystem::temp_directory_path() / "langchain_cpp_history_roundtrip.json";
    std::filesystem::remove(path);

    {
        FileChatMessageHistory history(path.string());
        history.add_message(Message::system("be nice"));
        history.add_message(Message::user("what is 2+2?"));
        history.add_message(
            Message::assistant_tool_calls({ToolCall{"call_1", "calculator", {{"a", 2}, {"b", 2}, {"op", "+"}}}}));
        history.add_message(Message::tool_result("call_1", "4"));
        history.add_message(Message::assistant("2+2 is 4."));
    }

    // A fresh instance pointed at the same path simulates a process restart.
    FileChatMessageHistory reloaded(path.string());
    auto messages = reloaded.messages();

    ASSERT_EQ(messages.size(), 5u);
    EXPECT_EQ(messages[0].role, MessageRole::System);
    EXPECT_EQ(messages[0].content, "be nice");
    EXPECT_EQ(messages[1].content, "what is 2+2?");

    ASSERT_TRUE(messages[2].has_tool_calls());
    EXPECT_EQ(messages[2].tool_calls[0].id, "call_1");
    EXPECT_EQ(messages[2].tool_calls[0].tool_name, "calculator");
    EXPECT_EQ(messages[2].tool_calls[0].arguments["op"], "+");

    EXPECT_EQ(messages[3].role, MessageRole::Tool);
    EXPECT_EQ(messages[3].tool_call_id, "call_1");
    EXPECT_EQ(messages[3].content, "4");

    EXPECT_EQ(messages[4].content, "2+2 is 4.");

    std::filesystem::remove(path);
}

TEST(FileChatMessageHistory, PersistsImageBearingMessage) {
    auto path = std::filesystem::temp_directory_path() / "langchain_cpp_history_images.json";
    std::filesystem::remove(path);

    {
        FileChatMessageHistory history(path.string());
        history.add_message(Message::user_with_images(
            "what's in this?",
            {ImageContent::from_url("https://example.com/cat.png"), ImageContent{ImageSourceType::Base64, "QUJD", "image/png"}}));
    }

    FileChatMessageHistory reloaded(path.string());
    auto messages = reloaded.messages();

    ASSERT_EQ(messages.size(), 1u);
    EXPECT_EQ(messages[0].content, "what's in this?");
    ASSERT_TRUE(messages[0].has_images());
    ASSERT_EQ(messages[0].images.size(), 2u);

    EXPECT_EQ(messages[0].images[0].source_type, ImageSourceType::Url);
    EXPECT_EQ(messages[0].images[0].data, "https://example.com/cat.png");

    EXPECT_EQ(messages[0].images[1].source_type, ImageSourceType::Base64);
    EXPECT_EQ(messages[0].images[1].data, "QUJD");
    EXPECT_EQ(messages[0].images[1].media_type, "image/png");

    std::filesystem::remove(path);
}

TEST(FileChatMessageHistory, ClearPersistsEmptyState) {
    auto path = std::filesystem::temp_directory_path() / "langchain_cpp_history_clear.json";
    std::filesystem::remove(path);

    {
        FileChatMessageHistory history(path.string());
        history.add_message(Message::user("hi"));
        history.clear();
    }

    FileChatMessageHistory reloaded(path.string());
    EXPECT_TRUE(reloaded.messages().empty());

    std::filesystem::remove(path);
}

TEST(ChatModelWithHistory, EachTurnSeesGrowingConversation) {
    auto model = std::make_shared<HistoryAwareModel>();
    auto history = std::make_shared<InMemoryChatMessageHistory>();
    ChatModelWithHistory chat(model, history);

    Message first = chat.invoke({Message::user("first")});
    EXPECT_EQ(first.content, "saw 1 message(s)"); // just this turn's user message

    Message second = chat.invoke({Message::user("second")});
    // history now holds: user "first", assistant reply, user "second" -> 3 messages
    EXPECT_EQ(second.content, "saw 3 message(s)");
}

TEST(ChatModelWithHistory, HistoryEndsUpWithFullTranscript) {
    auto model = std::make_shared<HistoryAwareModel>();
    auto history = std::make_shared<InMemoryChatMessageHistory>();
    ChatModelWithHistory chat(model, history);

    chat.invoke({Message::user("hello")});
    chat.invoke({Message::user("how are you")});

    auto messages = history->messages();
    ASSERT_EQ(messages.size(), 4u);
    EXPECT_EQ(messages[0].content, "hello");
    EXPECT_EQ(messages[0].role, MessageRole::User);
    EXPECT_EQ(messages[1].role, MessageRole::Assistant);
    EXPECT_EQ(messages[2].content, "how are you");
    EXPECT_EQ(messages[3].role, MessageRole::Assistant);
}
