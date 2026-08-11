// Demonstrates persisted conversation history: ChatModelWithHistory wraps
// a model so each call only needs *this turn's* new message, not the
// whole conversation. History is loaded from a FileChatMessageHistory, so
// running this program again (it reuses the same file in the system temp
// directory) picks up where the last run left off -- a real
// cross-process-restart test, not just an in-memory illusion.

#include "langchain/langchain.hpp"

#include <filesystem>
#include <iostream>

using namespace langchain;

int main() {
    std::string history_path = (std::filesystem::temp_directory_path() / "langchain_cpp_chat_history_demo.json").string();

    auto history = std::make_shared<llm::FileChatMessageHistory>(history_path);
    std::cout << "History file: " << history_path << "\n";
    std::cout << "Loaded " << history->messages().size() << " prior message(s).\n\n";

    auto model = std::make_shared<providers::MockChat>([](const std::vector<core::Message>& messages) {
        return "I've now seen " + std::to_string(messages.size()) + " message(s) in this conversation.";
    });

    llm::ChatModelWithHistory chat(model, history);

    core::Message reply = chat.invoke({core::Message::user("Hello, do you remember me?")});
    std::cout << "User:      Hello, do you remember me?\n";
    std::cout << "Assistant: " << reply.content << "\n\n";

    std::cout << "Run this example again -- the count will keep growing, since history persists in\n"
              << history_path << ".\n";
    return 0;
}
