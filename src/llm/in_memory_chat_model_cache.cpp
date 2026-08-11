#include "langchain/llm/in_memory_chat_model_cache.hpp"

namespace langchain::llm {

std::optional<core::Message> InMemoryChatModelCache::get(const std::string& key) {
    auto it = entries_.find(key);
    if (it == entries_.end()) {
        return std::nullopt;
    }
    return it->second;
}

void InMemoryChatModelCache::put(const std::string& key, const core::Message& value) { entries_[key] = value; }

} // namespace langchain::llm
