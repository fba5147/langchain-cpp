#pragma once

#include "langchain/core/message.hpp"

#include <vector>

namespace langchain::llm {

// Persists a single conversation's messages across calls -- the source
// of truth for "what has been said so far" in one conversation. Not a
// cache of *responses* (see ChatModelCache for that).
class ChatMessageHistory {
public:
    virtual ~ChatMessageHistory() = default;

    virtual std::vector<core::Message> messages() const = 0;
    virtual void add_message(const core::Message& message) = 0;
    virtual void clear() = 0;
};

} // namespace langchain::llm
