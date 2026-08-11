// Demonstrates a real provider call. Picks whichever API key is set in the
// environment (ANTHROPIC_API_KEY or OPENAI_API_KEY); falls back to MockChat
// with a note if neither is set, so the example always runs out of the box.

#include "langchain/langchain.hpp"

#include <cstdlib>
#include <iostream>
#include <memory>

using namespace langchain;

int main() {
    std::shared_ptr<llm::ChatModel> model;

    if (std::getenv("ANTHROPIC_API_KEY") != nullptr) {
        model = std::make_shared<providers::AnthropicChat>();
    } else if (std::getenv("OPENAI_API_KEY") != nullptr) {
        model = std::make_shared<providers::OpenAIChat>();
    } else {
        std::cout << "No ANTHROPIC_API_KEY or OPENAI_API_KEY set; using MockChat instead.\n"
                     "Set one of those environment variables to hit a real provider.\n\n";
        model = std::make_shared<providers::MockChat>("(mock) Move semantics let you transfer ownership of a "
                                                        "resource instead of copying it.");
    }

    std::vector<core::Message> messages{
        core::Message::system("You are a helpful, concise assistant."),
        core::Message::user("Explain C++ move semantics in one sentence."),
    };

    core::Message response = model->invoke(messages);
    std::cout << "[" << model->model_name() << "] " << response.content << '\n';

    return 0;
}
