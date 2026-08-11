#include "langchain/core/callbacks.hpp"

namespace langchain::core {

namespace {

template <typename Fn>
void dispatch(const std::vector<std::shared_ptr<CallbackHandler>>& handlers, Fn&& fn) {
    for (const auto& handler : handlers) {
        try {
            fn(*handler);
        } catch (...) {
            // An observability hook misbehaving shouldn't break the chain/agent it's observing.
        }
    }
}

} // namespace

void CallbackManager::add_handler(std::shared_ptr<CallbackHandler> handler) { handlers_.push_back(std::move(handler)); }

void CallbackManager::on_llm_start(const LlmStartEvent& event) {
    dispatch(handlers_, [&](CallbackHandler& handler) { handler.on_llm_start(event); });
}

void CallbackManager::on_llm_new_token(const LlmNewTokenEvent& event) {
    dispatch(handlers_, [&](CallbackHandler& handler) { handler.on_llm_new_token(event); });
}

void CallbackManager::on_llm_end(const LlmEndEvent& event) {
    dispatch(handlers_, [&](CallbackHandler& handler) { handler.on_llm_end(event); });
}

void CallbackManager::on_llm_error(const LlmErrorEvent& event) {
    dispatch(handlers_, [&](CallbackHandler& handler) { handler.on_llm_error(event); });
}

void CallbackManager::on_tool_start(const ToolStartEvent& event) {
    dispatch(handlers_, [&](CallbackHandler& handler) { handler.on_tool_start(event); });
}

void CallbackManager::on_tool_end(const ToolEndEvent& event) {
    dispatch(handlers_, [&](CallbackHandler& handler) { handler.on_tool_end(event); });
}

void CallbackManager::on_tool_error(const ToolErrorEvent& event) {
    dispatch(handlers_, [&](CallbackHandler& handler) { handler.on_tool_error(event); });
}

} // namespace langchain::core
