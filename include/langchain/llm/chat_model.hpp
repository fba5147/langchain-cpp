#pragma once

#include "langchain/core/message.hpp"
#include "langchain/core/runnable.hpp"
#include "langchain/tools/tool_registry.hpp"

#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

namespace langchain::llm {

// Base interface for anything that turns a conversation into a reply.
// A Runnable<vector<Message>, Message>, so it composes directly with
// prompt templates and output parsers via operator|.
class ChatModel : public core::Runnable<std::vector<core::Message>, core::Message> {
public:
    ~ChatModel() override = default;

    virtual std::string model_name() const = 0;

    // Returns a copy of this model that offers `tools` to the LLM as
    // function-calling candidates on every subsequent invoke(); a reply
    // that wants to call one comes back as Message::assistant_tool_calls.
    // Providers that don't support tool calling can leave the default,
    // which throws.
    virtual std::shared_ptr<ChatModel> bind_tools(std::shared_ptr<tools::ToolRegistry> registry) {
        (void)registry;
        throw std::logic_error(model_name() + " does not support tool calling");
    }
};

} // namespace langchain::llm
