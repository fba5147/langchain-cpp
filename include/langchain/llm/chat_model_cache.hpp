#pragma once

#include "langchain/core/message.hpp"

#include <optional>
#include <string>

namespace langchain::llm {

// Pluggable cache for ChatModel replies, keyed by whatever
// CachingChatModel derives from a request (model name + conversation +
// bound tools -- see CachingChatModel::make_cache_key).
class ChatModelCache {
public:
    virtual ~ChatModelCache() = default;

    virtual std::optional<core::Message> get(const std::string& key) = 0;
    virtual void put(const std::string& key, const core::Message& value) = 0;
};

} // namespace langchain::llm
