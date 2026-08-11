#include "langchain/llm/caching_chat_model.hpp"

#include <nlohmann/json.hpp>

namespace langchain::llm {

using json = nlohmann::json;

namespace {

json message_to_cache_json(const core::Message& message) {
    json calls = json::array();
    for (const auto& call : message.tool_calls) {
        calls.push_back({{"id", call.id}, {"name", call.tool_name}, {"arguments", call.arguments}});
    }
    // Images must be part of the key too: two requests differing only in
    // an attached image are different requests, even if the text matches.
    json images = json::array();
    for (const auto& image : message.images) {
        images.push_back({{"data", image.data}, {"media_type", image.media_type}});
    }
    return json{
        {"role", static_cast<int>(message.role)},
        {"content", message.content},
        {"tool_call_id", message.tool_call_id},
        {"tool_calls", calls},
        {"images", images},
    };
}

} // namespace

CachingChatModel::CachingChatModel(std::shared_ptr<ChatModel> inner, std::shared_ptr<ChatModelCache> cache)
    : inner_(std::move(inner)), cache_(std::move(cache)) {}

std::string CachingChatModel::make_cache_key(const std::vector<core::Message>& messages) const {
    json key = json::array();
    key.push_back(inner_->model_name());
    key.push_back(tools_signature_);
    for (const auto& message : messages) {
        key.push_back(message_to_cache_json(message));
    }
    return key.dump();
}

core::Message CachingChatModel::invoke(const std::vector<core::Message>& messages) {
    std::string key = make_cache_key(messages);
    if (auto cached = cache_->get(key)) {
        return *cached;
    }

    core::Message result = inner_->invoke(messages);
    cache_->put(key, result);
    return result;
}

void CachingChatModel::stream(const std::vector<core::Message>& messages, const StreamCallback& on_chunk) {
    std::string key = make_cache_key(messages);
    if (auto cached = cache_->get(key)) {
        on_chunk(StreamChunk{cached->content, true, *cached});
        return;
    }

    core::Message assembled;
    inner_->stream(messages, [&](const StreamChunk& chunk) {
        if (chunk.is_final) {
            assembled = chunk.message;
        }
        on_chunk(chunk);
    });
    cache_->put(key, assembled);
}

std::shared_ptr<ChatModel> CachingChatModel::bind_tools(std::shared_ptr<tools::ToolRegistry> registry) {
    std::string signature = registry->to_openai_tools_json().dump();
    auto wrapped = std::make_shared<CachingChatModel>(inner_->bind_tools(std::move(registry)), cache_);
    wrapped->tools_signature_ = std::move(signature);
    return wrapped;
}

} // namespace langchain::llm
