#include "langchain/prompts/chat_prompt_template.hpp"
#include "langchain/prompts/prompt_template.hpp"

#include <gtest/gtest.h>

using namespace langchain::core;
using namespace langchain::prompts;

TEST(PromptTemplate, SubstitutesVariables) {
    PromptTemplate prompt("Hello, {name}! You are {age} years old.");
    EXPECT_EQ(prompt.format({{"name", "Ada"}, {"age", "36"}}), "Hello, Ada! You are 36 years old.");
}

TEST(PromptTemplate, LeavesUnknownPlaceholdersAsIs) {
    PromptTemplate prompt("Hello, {name}!");
    EXPECT_EQ(prompt.format({}), "Hello, {name}!");
}

TEST(ChatPromptTemplate, SingleTemplateProducesOneUserMessage) {
    ChatPromptTemplate prompt("Explain {topic} in one sentence.");
    auto messages = prompt.format({{"topic", "RAII"}});
    ASSERT_EQ(messages.size(), 1u);
    EXPECT_EQ(messages[0].role, MessageRole::User);
    EXPECT_EQ(messages[0].content, "Explain RAII in one sentence.");
}

TEST(ChatPromptTemplate, MultiMessageTemplateRendersEachRole) {
    ChatPromptTemplate prompt({
        {MessageRole::System, "You are an expert in {language}."},
        {MessageRole::User, "Explain {topic}."},
    });
    auto messages = prompt.format({{"language", "C++"}, {"topic", "move semantics"}});
    ASSERT_EQ(messages.size(), 2u);
    EXPECT_EQ(messages[0].role, MessageRole::System);
    EXPECT_EQ(messages[0].content, "You are an expert in C++.");
    EXPECT_EQ(messages[1].role, MessageRole::User);
    EXPECT_EQ(messages[1].content, "Explain move semantics.");
}
