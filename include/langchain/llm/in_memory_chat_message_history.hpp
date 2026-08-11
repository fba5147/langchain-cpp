#pragma once

#include "langchain/llm/chat_message_history.hpp"

namespace langchain::llm {

class InMemoryChatMessageHistory : public ChatMessageHistory {
public:
    std::vector<core::Message> messages() const override;
    void add_message(const core::Message& message) override;
    void clear() override;

private:
    std::vector<core::Message> messages_;
};

} // namespace langchain::llm
