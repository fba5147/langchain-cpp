#pragma once

#include "langchain/llm/chat_model.hpp"
#include "langchain/llm/chat_model_cache.hpp"

#include <memory>
#include <string>

namespace langchain::llm {

// Wraps any ChatModel to cache replies by (model name, conversation,
// bound tools) -- identical requests skip the inner model entirely. The
// bound-tools component matters: caching "what's 2+2" without noticing
// that a calculator tool was later bound would risk serving a stale,
// tool-unaware answer for what is now a different question in substance.
//
// invoke() is a straightforward cache-or-call-and-store. stream() on a
// cache hit delivers the cached message as a single final chunk (there's
// nothing to stream -- the answer is already fully known); on a miss it
// delegates to the inner model's real stream() so incremental delivery
// isn't lost, and stores the assembled result once streaming finishes.
class CachingChatModel : public ChatModel {
public:
    CachingChatModel(std::shared_ptr<ChatModel> inner, std::shared_ptr<ChatModelCache> cache);

    core::Message invoke(const std::vector<core::Message>& messages) override;
    void stream(const std::vector<core::Message>& messages, const StreamCallback& on_chunk) override;
    std::string model_name() const override { return inner_->model_name(); }
    std::shared_ptr<ChatModel> bind_tools(std::shared_ptr<tools::ToolRegistry> registry) override;

private:
    std::string make_cache_key(const std::vector<core::Message>& messages) const;

    std::shared_ptr<ChatModel> inner_;
    std::shared_ptr<ChatModelCache> cache_;
    std::string tools_signature_;
};

} // namespace langchain::llm
