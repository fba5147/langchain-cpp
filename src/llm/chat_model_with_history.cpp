#include "langchain/llm/chat_model_with_history.hpp"

namespace langchain::llm {

ChatModelWithHistory::ChatModelWithHistory(std::shared_ptr<ChatModel> model,
                                            std::shared_ptr<ChatMessageHistory> history)
    : model_(std::move(model)), history_(std::move(history)) {}

core::Message ChatModelWithHistory::invoke(const std::vector<core::Message>& new_messages) {
    std::vector<core::Message> full = history_->messages();
    full.insert(full.end(), new_messages.begin(), new_messages.end());

    core::Message reply = model_->invoke(full);

    for (const auto& message : new_messages) {
        history_->add_message(message);
    }
    history_->add_message(reply);

    return reply;
}

} // namespace langchain::llm
