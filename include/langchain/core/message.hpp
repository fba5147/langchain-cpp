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

enum class ImageSourceType {
    Url,    // a remote (or data:) URL, sent to the provider as-is
    Base64, // inline base64-encoded bytes
};

// A single image attached to a Message. Not every provider's wire-format
// code encodes this yet -- see ChatModel implementations for which ones
// do (as of this writing, only OpenAIChat/AzureOpenAIChat; AnthropicChat
// and GeminiChat throw rather than silently drop an attached image).
struct ImageContent {
    ImageSourceType source_type;
    std::string data;       // the URL, or the base64-encoded bytes
    std::string media_type; // e.g. "image/png" -- required for Base64, informative for Url

    static ImageContent from_url(std::string url, std::string media_type = "") {
        return ImageContent{ImageSourceType::Url, std::move(url), std::move(media_type)};
    }

    // Reads the file at `path`, base64-encodes its bytes, and guesses
    // media_type from the file extension (.png/.jpg/.jpeg/.gif/.webp) if
    // not given explicitly. Throws std::runtime_error if the file can't
    // be read, or if the extension is unrecognized and no media_type was
    // given.
    static ImageContent from_file(const std::string& path, std::string media_type = "");
};

struct Message {
    MessageRole role;
    std::string content;
    std::vector<ToolCall> tool_calls = {};
    std::string tool_call_id = {};
    std::vector<ImageContent> images = {};

    static Message system(std::string content) { return Message{MessageRole::System, std::move(content)}; }
    static Message user(std::string content) { return Message{MessageRole::User, std::move(content)}; }
    static Message assistant(std::string content) { return Message{MessageRole::Assistant, std::move(content)}; }

    // A user turn with one or more images attached alongside the text.
    static Message user_with_images(std::string text, std::vector<ImageContent> images) {
        Message message{MessageRole::User, std::move(text)};
        message.images = std::move(images);
        return message;
    }

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
    bool has_images() const { return !images.empty(); }
};

} // namespace langchain::core
