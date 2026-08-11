#pragma once

#include "langchain/llm/chat_model_cache.hpp"

#include <unordered_map>

namespace langchain::llm {

// A simple process-local cache; entries never expire and there's no size
// bound -- fine for development/testing/demos, not for a long-running
// production process fielding unbounded unique inputs.
class InMemoryChatModelCache : public ChatModelCache {
public:
    std::optional<core::Message> get(const std::string& key) override;
    void put(const std::string& key, const core::Message& value) override;

    std::size_t size() const { return entries_.size(); }

private:
    std::unordered_map<std::string, core::Message> entries_;
};

} // namespace langchain::llm
