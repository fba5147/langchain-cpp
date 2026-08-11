#pragma once

#include "langchain/core/runnable.hpp"
#include "langchain/llm/chat_message_history.hpp"
#include "langchain/llm/chat_model.hpp"

#include <memory>

namespace langchain::llm {

// Wraps a ChatModel so each invoke() only needs to pass *this turn's* new
// message(s) (typically one user message) -- not the whole conversation.
// History is loaded from a ChatMessageHistory, the new turn is appended
// to it, the inner model sees history + new turn, and the reply is
// appended back before being returned.
//
// Deliberately not a ChatModel itself: a ChatModel's invoke() contract
// everywhere else in this library means "here is the full conversation,"
// the opposite of what this class expects as input. Making it a ChatModel
// would invite exactly that mix-up -- e.g. inside AgentExecutor, which
// always passes the full running conversation on every step.
class ChatModelWithHistory : public core::Runnable<std::vector<core::Message>, core::Message> {
public:
    ChatModelWithHistory(std::shared_ptr<ChatModel> model, std::shared_ptr<ChatMessageHistory> history);

    core::Message invoke(const std::vector<core::Message>& new_messages) override;

private:
    std::shared_ptr<ChatModel> model_;
    std::shared_ptr<ChatMessageHistory> history_;
};

} // namespace langchain::llm
