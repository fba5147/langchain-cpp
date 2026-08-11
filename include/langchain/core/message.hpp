#pragma once

#include <nlohmann/json.hpp>
#include <string>
#include <utility>
#include <vector>

namespace langchain::core {

enum class MessageRole {
    System,
    User,
    Assistant,
    Tool,
};

// The API role string a provider expects on the wire (e.g. "user", "assistant").
std::string to_api_role(MessageRole role);

// A request an assistant Message makes to invoke a Tool by name with the
// given (already-parsed) arguments. `id` correlates the eventual tool
// result back to this specific call, since a single assistant turn can
// request several.
struct ToolCall {
    std::string id;
    std::string tool_name;
    nlohmann::json arguments;
};

struct Message {
    MessageRole role;
    std::string content;
    std::vector<ToolCall> tool_calls = {};
    std::string tool_call_id = {};

    static Message system(std::string content) { return Message{MessageRole::System, std::move(content)}; }
    static Message user(std::string content) { return Message{MessageRole::User, std::move(content)}; }
    static Message assistant(std::string content) { return Message{MessageRole::Assistant, std::move(content)}; }

    // An assistant turn that asks for one or more tools to be called.
    // `content` is usually empty, but some providers attach reasoning text
    // alongside the tool calls.
    static Message assistant_tool_calls(std::vector<ToolCall> tool_calls, std::string content = "") {
        Message message{MessageRole::Assistant, std::move(content)};
        message.tool_calls = std::move(tool_calls);
        return message;
    }

    // A tool's result, correlated back to the ToolCall::id that requested it.
    static Message tool_result(std::string tool_call_id, std::string content) {
        Message message{MessageRole::Tool, std::move(content)};
        message.tool_call_id = std::move(tool_call_id);
        return message;
    }

    bool has_tool_calls() const { return !tool_calls.empty(); }
};

} // namespace langchain::core
