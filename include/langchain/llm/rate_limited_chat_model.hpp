#pragma once

#include "langchain/llm/chat_model.hpp"
#include "langchain/llm/rate_limiter.hpp"

#include <memory>

namespace langchain::llm {

// Wraps any ChatModel to throttle invoke()/stream() calls via a shared
// RateLimiter -- e.g. to stay under a provider's requests-per-second
// quota. Pass the same RateLimiter to multiple RateLimitedChatModel
// instances (even wrapping different models) to throttle a whole
// application against one combined limit, not just one model.
class RateLimitedChatModel : public ChatModel {
public:
    RateLimitedChatModel(std::shared_ptr<ChatModel> inner, std::shared_ptr<RateLimiter> limiter);

    core::Message invoke(const std::vector<core::Message>& messages) override;
    void stream(const std::vector<core::Message>& messages, const StreamCallback& on_chunk) override;
    std::string model_name() const override { return inner_->model_name(); }
    std::shared_ptr<ChatModel> bind_tools(std::shared_ptr<tools::ToolRegistry> registry) override;

private:
    std::shared_ptr<ChatModel> inner_;
    std::shared_ptr<RateLimiter> limiter_;
};

} // namespace langchain::llm
