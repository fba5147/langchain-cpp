#include "langchain/core/runnable.hpp"
#include "langchain/parsers/string_parser.hpp"
#include "langchain/prompts/chat_prompt_template.hpp"
#include "langchain/providers/mock/mock_chat.hpp"

#include <gtest/gtest.h>

using namespace langchain::core;
using namespace langchain::prompts;
using namespace langchain::parsers;
using namespace langchain::providers;

TEST(Runnable, PipeComposesPromptModelParser) {
    auto prompt = std::make_shared<ChatPromptTemplate>("Say hi to {name}.");
    auto model = std::make_shared<MockChat>("Hi there!");
    auto parser = std::make_shared<StrOutputParser>();

    auto chain = prompt | model | parser;

    EXPECT_EQ(chain->invoke({{"name", "World"}}), "Hi there!");
}

TEST(Runnable, BatchInvokesEachInputIndependently) {
    auto model = std::make_shared<MockChat>(
        [](const std::vector<Message>& messages) { return "echo: " + messages.back().content; });

    auto results = model->batch({
        {Message::user("a")},
        {Message::user("b")},
    });

    ASSERT_EQ(results.size(), 2u);
    EXPECT_EQ(results[0].content, "echo: a");
    EXPECT_EQ(results[1].content, "echo: b");
}

TEST(Runnable, LambdaComposesIntoChain) {
    auto shout = make_runnable_lambda<std::string, std::string>(
        [](const std::string& input) { return input + "!!!"; });
    auto model = std::make_shared<MockChat>(
        [](const std::vector<Message>& messages) { return messages.back().content; });
    auto to_messages = make_runnable_lambda<std::string, std::vector<Message>>(
        [](const std::string& input) { return std::vector<Message>{Message::user(input)}; });
    auto parser = std::make_shared<StrOutputParser>();

    auto chain = shout | to_messages | model | parser;

    EXPECT_EQ(chain->invoke("hello"), "hello!!!");
}
