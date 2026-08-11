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

TEST(RunnablePassthrough, ReturnsInputUnchanged) {
    RunnablePassthrough<std::string> passthrough;
    EXPECT_EQ(passthrough.invoke("hello"), "hello");
}

TEST(RunnableParallel, CollectsBranchResultsByKey) {
    RunnableParallel<std::string, std::string>::Branches branches;
    branches["upper"] = make_runnable_lambda<std::string, std::string>(
        [](const std::string& input) { return input + "!"; });
    branches["question"] = std::make_shared<RunnablePassthrough<std::string>>();

    RunnableParallel<std::string, std::string> parallel(std::move(branches));
    auto result = parallel.invoke("hi");

    EXPECT_EQ(result["upper"], "hi!");
    EXPECT_EQ(result["question"], "hi");
}

TEST(RunnableParallel, ComposesIntoPromptTemplateViaOperatorPipe) {
    // RunnableParallel<Input, std::string>'s OutputType is exactly
    // PromptValues, so it should pipe directly into a ChatPromptTemplate.
    RunnableParallel<std::string, std::string>::Branches branches;
    branches["context"] = make_runnable_lambda<std::string, std::string>(
        [](const std::string&) { return "the context"; });
    branches["question"] = std::make_shared<RunnablePassthrough<std::string>>();
    auto parallel = std::make_shared<RunnableParallel<std::string, std::string>>(std::move(branches));

    auto prompt = std::make_shared<langchain::prompts::ChatPromptTemplate>(
        std::vector<std::pair<MessageRole, std::string>>{
            {MessageRole::User, "Context: {context}\nQuestion: {question}"},
        });

    auto chain = parallel | prompt;
    auto messages = chain->invoke("what is RAII?");

    ASSERT_EQ(messages.size(), 1u);
    EXPECT_EQ(messages[0].content, "Context: the context\nQuestion: what is RAII?");
}

TEST(RunnableBranch, RoutesToFirstMatchingPredicate) {
    RunnableBranch<std::string, std::string>::Predicate is_greeting = [](const std::string& input) {
        return input == "hi";
    };
    auto greeting_branch =
        make_runnable_lambda<std::string, std::string>([](const std::string&) { return "hello!"; });
    auto default_branch =
        make_runnable_lambda<std::string, std::string>([](const std::string& input) { return "echo: " + input; });

    RunnableBranch<std::string, std::string> branch({{is_greeting, greeting_branch}}, default_branch);

    EXPECT_EQ(branch.invoke("hi"), "hello!");
    EXPECT_EQ(branch.invoke("something else"), "echo: something else");
}
