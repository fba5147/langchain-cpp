#pragma once

#include "langchain/core/message.hpp"

#include <nlohmann/json.hpp>

#include <functional>
#include <map>
#include <string>
#include <string_view>

// Shared OpenAI-compatible chat wire-format helpers, used by both
// OpenAIChat and AzureOpenAIChat -- Azure's chat completions request and
// response body shape is identical to OpenAI's; only the URL and auth
// header differ. Internal implementation detail: lives under src/, not
// include/, and is not part of the public API.
namespace langchain::providers::detail {

nlohmann::json message_to_openai_json(const core::Message& message);
core::Message parse_openai_message(const nlohmann::json& message_json);

// Incrementally parses an OpenAI-compatible SSE chat-completion stream
// (`data: {...}` lines, terminated by a literal `data: [DONE]`),
// accumulating text and tool-call deltas across chunks. OpenAI fragments
// a tool call's `arguments` string across multiple deltas identified by
// index; Ollama has been observed sending each tool call whole in a
// single delta -- concatenating fragments handles both correctly (a
// single whole fragment concatenates to itself).
class OpenAiStreamParser {
public:
    // Called with raw bytes as they arrive over the wire; may be invoked
    // many times with partial lines (buffers internally across calls).
    // Calls on_delta for each text delta seen. Returns false once the
    // terminating "[DONE]" line has been seen (safe to stop feeding).
    bool feed(std::string_view data, const std::function<void(const std::string&)>& on_delta);

    // Assembles the final Message from everything accumulated so far.
    // Call once the stream has ended.
    core::Message finish() const;

private:
    void process_line(const std::string& line, const std::function<void(const std::string&)>& on_delta);

    std::string buffer_;
    std::string text_;
    bool done_ = false;

    struct ToolCallAccumulator {
        std::string id;
        std::string name;
        std::string arguments;
    };
    std::map<int, ToolCallAccumulator> tool_calls_;
};

} // namespace langchain::providers::detail
