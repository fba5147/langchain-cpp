#include "langchain/callbacks/callbacking_chat_model.hpp"
#include "langchain/callbacks/callbacking_tool.hpp"
#include "langchain/callbacks/console_callback_handler.hpp"
#include "langchain/core/callbacks.hpp"
#include "langchain/providers/mock/mock_chat.hpp"
#include "langchain/tools/function_tool.hpp"

#include <gtest/gtest.h>

#include <sstream>
#include <stdexcept>

using namespace langchain::core;
using namespace langchain::llm;
using namespace langchain::providers;
using namespace langchain::tools;
using namespace langchain::callbacks;

namespace {

class RecordingCallbackHandler : public CallbackHandler {
public:
    std::vector<std::string> events;

    void on_llm_start(const LlmStartEvent& event) override { events.push_back("llm_start:" + event.model_name); }
    void on_llm_new_token(const LlmNewTokenEvent& event) override { events.push_back("llm_new_token:" + event.delta); }
    void on_llm_end(const LlmEndEvent& event) override { events.push_back("llm_end:" + event.result.content); }
    void on_llm_error(const LlmErrorEvent& event) override { events.push_back("llm_error:" + event.error); }
    void on_tool_start(const ToolStartEvent& event) override { events.push_back("tool_start:" + event.tool_name); }
    void on_tool_end(const ToolEndEvent& event) override { events.push_back("tool_end:" + event.tool_name); }
    void on_tool_error(const ToolErrorEvent& event) override { events.push_back("tool_error:" + event.tool_name); }
};

class ThrowingChatModel : public ChatModel {
public:
    Message invoke(const std::vector<Message>&) override { throw std::runtime_error("boom"); }
    std::string model_name() const override { return "throwing-model"; }
};

} // namespace

TEST(CallbackManager, DispatchesToMultipleHandlersInOrder) {
    auto first = std::make_shared<RecordingCallbackHandler>();
    auto second = std::make_shared<RecordingCallbackHandler>();

    CallbackManager manager;
    manager.add_handler(first);
    manager.add_handler(second);

    manager.on_llm_start({"model", {}});

    ASSERT_EQ(first->events.size(), 1u);
    ASSERT_EQ(second->events.size(), 1u);
    EXPECT_EQ(first->events[0], "llm_start:model");
    EXPECT_EQ(second->events[0], "llm_start:model");
}

TEST(CallbackManager, AHandlerThrowingDoesNotStopOtherHandlers) {
    class ThrowingHandler : public CallbackHandler {
    public:
        void on_llm_start(const LlmStartEvent&) override { throw std::runtime_error("handler bug"); }
    };

    auto throwing = std::make_shared<ThrowingHandler>();
    auto recording = std::make_shared<RecordingCallbackHandler>();

    CallbackManager manager;
    manager.add_handler(throwing);
    manager.add_handler(recording);

    EXPECT_NO_THROW(manager.on_llm_start({"model", {}}));
    ASSERT_EQ(recording->events.size(), 1u);
}

TEST(CallbackingChatModel, FiresStartThenEndOnSuccessfulInvoke) {
    auto recording = std::make_shared<RecordingCallbackHandler>();
    auto manager = std::make_shared<CallbackManager>();
    manager->add_handler(recording);

    CallbackingChatModel model(std::make_shared<MockChat>("fixed reply"), manager);
    Message result = model.invoke({Message::user("hi")});

    EXPECT_EQ(result.content, "fixed reply");
    ASSERT_EQ(recording->events.size(), 2u);
    EXPECT_EQ(recording->events[0], "llm_start:mock-chat");
    EXPECT_EQ(recording->events[1], "llm_end:fixed reply");
}

TEST(CallbackingChatModel, FiresErrorAndRethrowsOnFailingInvoke) {
    auto recording = std::make_shared<RecordingCallbackHandler>();
    auto manager = std::make_shared<CallbackManager>();
    manager->add_handler(recording);

    CallbackingChatModel model(std::make_shared<ThrowingChatModel>(), manager);

    EXPECT_THROW(model.invoke({Message::user("hi")}), std::runtime_error);
    ASSERT_EQ(recording->events.size(), 2u);
    EXPECT_EQ(recording->events[0], "llm_start:throwing-model");
    EXPECT_EQ(recording->events[1], "llm_error:boom");
}

TEST(CallbackingChatModel, StreamFiresStartNewTokenThenEndAndForwardsChunksToCaller) {
    auto recording = std::make_shared<RecordingCallbackHandler>();
    auto manager = std::make_shared<CallbackManager>();
    manager->add_handler(recording);

    CallbackingChatModel model(std::make_shared<MockChat>("two words"), manager);

    std::vector<StreamChunk> forwarded;
    model.stream({Message::user("hi")}, [&](const StreamChunk& chunk) { forwarded.push_back(chunk); });

    // MockChat streams word by word: "two", " words", then a final chunk.
    ASSERT_EQ(forwarded.size(), 3u);
    EXPECT_TRUE(forwarded.back().is_final);

    ASSERT_EQ(recording->events.size(), 4u); // start + 2 new_token + end
    EXPECT_EQ(recording->events[0], "llm_start:mock-chat");
    EXPECT_EQ(recording->events[1], "llm_new_token:two");
    EXPECT_EQ(recording->events[2], "llm_new_token: words");
    EXPECT_EQ(recording->events[3], "llm_end:two words");
}

TEST(CallbackingChatModel, BindToolsWrapsToolBoundInnerModel) {
    auto manager = std::make_shared<CallbackManager>();
    CallbackingChatModel model(std::make_shared<MockChat>("reply"), manager);

    auto bound = model.bind_tools(std::make_shared<ToolRegistry>());

    EXPECT_EQ(bound->model_name(), "mock-chat");
    EXPECT_EQ(bound->invoke({Message::user("hi")}).content, "reply");
}

TEST(CallbackingTool, FiresStartThenEndOnSuccess) {
    auto recording = std::make_shared<RecordingCallbackHandler>();
    auto manager = std::make_shared<CallbackManager>();
    manager->add_handler(recording);

    auto inner = std::make_shared<FunctionTool>(
        "add", "adds two numbers",
        [](const nlohmann::json& input) -> nlohmann::json { return input.at("a").get<int>() + input.at("b").get<int>(); });
    CallbackingTool tool(inner, manager);

    auto result = tool.call({{"a", 2}, {"b", 3}});

    ASSERT_TRUE(result.ok());
    EXPECT_EQ(result.value(), 5);
    ASSERT_EQ(recording->events.size(), 2u);
    EXPECT_EQ(recording->events[0], "tool_start:add");
    EXPECT_EQ(recording->events[1], "tool_end:add");
}

TEST(CallbackingTool, FiresStartThenErrorWhenInnerToolReturnsError) {
    auto recording = std::make_shared<RecordingCallbackHandler>();
    auto manager = std::make_shared<CallbackManager>();
    manager->add_handler(recording);

    auto inner = std::make_shared<FunctionTool>(
        "fail", "always fails", [](const nlohmann::json&) -> nlohmann::json { throw std::invalid_argument("boom"); });
    CallbackingTool tool(inner, manager);

    auto result = tool.call({});

    ASSERT_FALSE(result.ok());
    ASSERT_EQ(recording->events.size(), 2u);
    EXPECT_EQ(recording->events[0], "tool_start:fail");
    EXPECT_EQ(recording->events[1], "tool_error:fail");
}

TEST(ConsoleCallbackHandler, WritesReadableOutput) {
    std::ostringstream out;
    ConsoleCallbackHandler handler(out);

    handler.on_llm_start({"mock-chat", {Message::user("hi")}});
    handler.on_llm_end({"mock-chat", Message::assistant("hello")});
    handler.on_tool_start({"calculator", {{"a", 1}}});
    handler.on_tool_end({"calculator", 2});

    std::string text = out.str();
    EXPECT_NE(text.find("mock-chat"), std::string::npos);
    EXPECT_NE(text.find("hello"), std::string::npos);
    EXPECT_NE(text.find("calculator"), std::string::npos);
}
