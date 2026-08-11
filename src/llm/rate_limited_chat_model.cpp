#include "langchain/llm/rate_limited_chat_model.hpp"

namespace langchain::llm {

RateLimitedChatModel::RateLimitedChatModel(std::shared_ptr<ChatModel> inner, std::shared_ptr<RateLimiter> limiter)
    : inner_(std::move(inner)), limiter_(std::move(limiter)) {}

core::Message RateLimitedChatModel::invoke(const std::vector<core::Message>& messages) {
    limiter_->acquire();
    return inner_->invoke(messages);
}

void RateLimitedChatModel::stream(const std::vector<core::Message>& messages, const StreamCallback& on_chunk) {
    limiter_->acquire();
    inner_->stream(messages, on_chunk);
}

std::shared_ptr<ChatModel> RateLimitedChatModel::bind_tools(std::shared_ptr<tools::ToolRegistry> registry) {
    return std::make_shared<RateLimitedChatModel>(inner_->bind_tools(std::move(registry)), limiter_);
}

} // namespace langchain::llm
