#pragma once

#include "langchain/core/message.hpp"

#include <nlohmann/json.hpp>

#include <memory>
#include <string>
#include <vector>

namespace langchain::core {

struct LlmStartEvent {
    std::string model_name;
    std::vector<Message> messages;
};
struct LlmNewTokenEvent {
    std::string model_name;
    std::string delta;
};
struct LlmEndEvent {
    std::string model_name;
    Message result;
};
struct LlmErrorEvent {
    std::string model_name;
    std::string error;
};

struct ToolStartEvent {
    std::string tool_name;
    nlohmann::json input;
};
struct ToolEndEvent {
    std::string tool_name;
    nlohmann::json output;
};
struct ToolErrorEvent {
    std::string tool_name;
    std::string error;
};

// Override whichever hooks you care about; the rest no-op. Mirrors
// LangChain's BaseCallbackHandler, scoped to what this library can emit
// without invasive changes to every Runnable -- there's no generic
// on_chain_start/end here. LLM and tool calls get instrumented by wrapping
// a ChatModel/Tool in CallbackingChatModel/CallbackingTool
// (langchain/callbacks/), not by changing the wrapped object itself.
class CallbackHandler {
public:
    virtual ~CallbackHandler() = default;

    virtual void on_llm_start(const LlmStartEvent&) {}
    virtual void on_llm_new_token(const LlmNewTokenEvent&) {}
    virtual void on_llm_end(const LlmEndEvent&) {}
    virtual void on_llm_error(const LlmErrorEvent&) {}

    virtual void on_tool_start(const ToolStartEvent&) {}
    virtual void on_tool_end(const ToolEndEvent&) {}
    virtual void on_tool_error(const ToolErrorEvent&) {}
};

// Fans out events to every registered handler, in registration order. A
// handler that throws does not stop the others from running (the
// exception is swallowed) -- an observability hook misbehaving shouldn't
// break the actual chain/agent it's observing.
class CallbackManager {
public:
    void add_handler(std::shared_ptr<CallbackHandler> handler);

    void on_llm_start(const LlmStartEvent& event);
    void on_llm_new_token(const LlmNewTokenEvent& event);
    void on_llm_end(const LlmEndEvent& event);
    void on_llm_error(const LlmErrorEvent& event);

    void on_tool_start(const ToolStartEvent& event);
    void on_tool_end(const ToolEndEvent& event);
    void on_tool_error(const ToolErrorEvent& event);

private:
    std::vector<std::shared_ptr<CallbackHandler>> handlers_;
};

} // namespace langchain::core
