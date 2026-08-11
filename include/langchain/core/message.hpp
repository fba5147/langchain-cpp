#pragma once

#include <string>

namespace langchain::core {

enum class MessageRole {
    System,
    User,
    Assistant,
    Tool,
};

// The API role string a provider expects on the wire (e.g. "user", "assistant").
std::string to_api_role(MessageRole role);

struct Message {
    MessageRole role;
    std::string content;

    static Message system(std::string content) { return Message{MessageRole::System, std::move(content)}; }
    static Message user(std::string content) { return Message{MessageRole::User, std::move(content)}; }
    static Message assistant(std::string content) { return Message{MessageRole::Assistant, std::move(content)}; }
    static Message tool(std::string content) { return Message{MessageRole::Tool, std::move(content)}; }
};

} // namespace langchain::core
