#include "langchain/llm/in_memory_chat_message_history.hpp"

namespace langchain::llm {

std::vector<core::Message> InMemoryChatMessageHistory::messages() const { return messages_; }

void InMemoryChatMessageHistory::add_message(const core::Message& message) { messages_.push_back(message); }

void InMemoryChatMessageHistory::clear() { messages_.clear(); }

} // namespace langchain::llm
