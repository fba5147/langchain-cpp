// Demonstrates the LCEL-style `prompt | model | parser` chain end-to-end
// with no network calls, using MockChat in place of a real provider.

#include "langchain/langchain.hpp"

#include <iostream>

using namespace langchain;

int main() {
    auto prompt = std::make_shared<prompts::ChatPromptTemplate>(std::vector<std::pair<core::MessageRole, std::string>>{
        {core::MessageRole::System, "You are an expert in {language}."},
        {core::MessageRole::User, "Explain {topic} in one sentence."},
    });

    auto model = std::make_shared<providers::MockChat>([](const std::vector<core::Message>& messages) {
        return "(mock) A crisp one-sentence answer about: " + messages.back().content;
    });

    auto parser = std::make_shared<parsers::StrOutputParser>();

    auto chain = prompt | model | parser;

    std::string answer = chain->invoke({{"language", "C++"}, {"topic", "RAII"}});
    std::cout << answer << '\n';

    return 0;
}
