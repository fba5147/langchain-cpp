#include "langchain/callbacks/callbacking_chat_model.hpp"

#include <exception>

namespace langchain::callbacks {

CallbackingChatModel::CallbackingChatModel(std::shared_ptr<llm::ChatModel> inner,
                                            std::shared_ptr<core::CallbackManager> callbacks)
    : inner_(std::move(inner)), callbacks_(std::move(callbacks)) {}

core::Message CallbackingChatModel::invoke(const std::vector<core::Message>& messages) {
    callbacks_->on_llm_start({inner_->model_name(), messages});
    try {
        core::Message result = inner_->invoke(messages);
        callbacks_->on_llm_end({inner_->model_name(), result});
        return result;
    } catch (const std::exception& e) {
        callbacks_->on_llm_error({inner_->model_name(), e.what()});
        throw;
    }
}

void CallbackingChatModel::stream(const std::vector<core::Message>& messages, const StreamCallback& on_chunk) {
    callbacks_->on_llm_start({inner_->model_name(), messages});
    try {
        inner_->stream(messages, [this, &on_chunk](const llm::StreamChunk& chunk) {
            if (chunk.is_final) {
                callbacks_->on_llm_end({inner_->model_name(), chunk.message});
            } else {
                callbacks_->on_llm_new_token({inner_->model_name(), chunk.delta});
            }
            on_chunk(chunk);
        });
    } catch (const std::exception& e) {
        callbacks_->on_llm_error({inner_->model_name(), e.what()});
        throw;
    }
}

std::shared_ptr<llm::ChatModel> CallbackingChatModel::bind_tools(std::shared_ptr<tools::ToolRegistry> registry) {
    return std::make_shared<CallbackingChatModel>(inner_->bind_tools(std::move(registry)), callbacks_);
}

} // namespace langchain::callbacks
