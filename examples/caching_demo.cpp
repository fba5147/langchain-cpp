// Demonstrates CachingChatModel: identical requests skip the inner model
// entirely. Uses a model that reports how many times it was actually
// called, so the effect is directly visible rather than just asserted.
//
// Fully offline -- no real provider needed.

#include "langchain/langchain.hpp"

#include <iostream>

using namespace langchain;

namespace {

class CountingChatModel : public llm::ChatModel {
public:
    int calls = 0;

    core::Message invoke(const std::vector<core::Message>& messages) override {
        ++calls;
        return core::Message::assistant("Response #" + std::to_string(calls) + " to: " + messages.back().content);
    }

    std::string model_name() const override { return "counting-model"; }
};

} // namespace

int main() {
    auto inner = std::make_shared<CountingChatModel>();
    auto cache = std::make_shared<llm::InMemoryChatModelCache>();
    llm::CachingChatModel model(inner, cache);

    core::Message first = model.invoke({core::Message::user("What is RAII?")});
    std::cout << "1st call:                    " << first.content << " (inner called " << inner->calls
              << " time(s) so far)\n";

    core::Message second = model.invoke({core::Message::user("What is RAII?")});
    std::cout << "2nd call, same question:     " << second.content << " (inner called " << inner->calls
              << " time(s) so far)\n";

    core::Message third = model.invoke({core::Message::user("What is a vector database?")});
    std::cout << "3rd call, different question: " << third.content << " (inner called " << inner->calls
              << " time(s) so far)\n";

    return 0;
}
