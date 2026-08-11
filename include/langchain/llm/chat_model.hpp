#pragma once

#include "langchain/core/message.hpp"
#include "langchain/core/runnable.hpp"
#include "langchain/tools/tool_registry.hpp"

#include <functional>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

namespace langchain::llm {

// One piece of a streamed reply. `delta` is the incremental text for
// non-final chunks; the last chunk delivered to a stream() callback has
// is_final=true, an empty delta, and the fully assembled `message`
// (content plus any tool_calls).
struct StreamChunk {
    std::string delta;
    bool is_final = false;
    core::Message message;
};

// Base interface for anything that turns a conversation into a reply.
// A Runnable<vector<Message>, Message>, so it composes directly with
// prompt templates and output parsers via operator|.
class ChatModel : public core::Runnable<std::vector<core::Message>, core::Message> {
public:
    using StreamCallback = std::function<void(const StreamChunk&)>;

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

    // Streams the reply: on_chunk is called once per incremental text
    // delta, then once more with is_final=true and the fully assembled
    // Message. This default synthesizes that single final chunk from
    // invoke(), so every ChatModel supports stream() correctly even if it
    // can't do so incrementally; providers with real incremental
    // streaming (OpenAIChat, AzureOpenAIChat) override this.
    virtual void stream(const std::vector<core::Message>& messages, const StreamCallback& on_chunk) {
        core::Message result = invoke(messages);
        on_chunk(StreamChunk{result.content, true, result});
    }
};

} // namespace langchain::llm
