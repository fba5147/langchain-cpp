#pragma once

#include "langchain/core/callbacks.hpp"
#include "langchain/llm/chat_model.hpp"

#include <memory>

namespace langchain::callbacks {

// Wraps any ChatModel to fire CallbackManager events around invoke() and
// stream(), without changing the wrapped model at all -- so any existing
// provider gets observability just by being wrapped, and bind_tools()
// composes normally (it wraps the tool-bound copy of the inner model).
class CallbackingChatModel : public llm::ChatModel {
public:
    CallbackingChatModel(std::shared_ptr<llm::ChatModel> inner, std::shared_ptr<core::CallbackManager> callbacks);

    core::Message invoke(const std::vector<core::Message>& messages) override;
    void stream(const std::vector<core::Message>& messages, const StreamCallback& on_chunk) override;
    std::string model_name() const override { return inner_->model_name(); }
    std::shared_ptr<llm::ChatModel> bind_tools(std::shared_ptr<tools::ToolRegistry> registry) override;

private:
    std::shared_ptr<llm::ChatModel> inner_;
    std::shared_ptr<core::CallbackManager> callbacks_;
};

} // namespace langchain::callbacks
