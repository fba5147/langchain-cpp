// Demonstrates a real provider call. Picks whichever API key is set in the
// environment (ANTHROPIC_API_KEY, OPENAI_API_KEY, or GOOGLE_API_KEY);
// falls back to MockChat with a note if none are set, so the example
// always runs out of the box. Loads a .env file from the current working
// directory first (see .env.example) -- run this from the repo root.

#include "langchain/langchain.hpp"

#include <cstdlib>
#include <iostream>
#include <memory>

using namespace langchain;

int main() {
    core::load_dotenv();

    std::shared_ptr<llm::ChatModel> model;

    if (std::getenv("ANTHROPIC_API_KEY") != nullptr) {
        model = std::make_shared<providers::AnthropicChat>();
    } else if (std::getenv("OPENAI_API_KEY") != nullptr) {
        model = std::make_shared<providers::OpenAIChat>();
    } else if (std::getenv("GOOGLE_API_KEY") != nullptr) {
        model = std::make_shared<providers::GeminiChat>();
    } else {
        std::cout << "No ANTHROPIC_API_KEY, OPENAI_API_KEY, or GOOGLE_API_KEY set; using MockChat instead.\n"
                     "Set one of those (or create a .env file, see .env.example) to hit a real provider.\n\n";
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
