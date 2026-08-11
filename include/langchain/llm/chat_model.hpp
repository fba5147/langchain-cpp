#pragma once

#include "langchain/core/message.hpp"
#include "langchain/core/runnable.hpp"

#include <string>
#include <vector>

namespace langchain::llm {

// Base interface for anything that turns a conversation into a reply.
// A Runnable<vector<Message>, Message>, so it composes directly with
// prompt templates and output parsers via operator|.
class ChatModel : public core::Runnable<std::vector<core::Message>, core::Message> {
public:
    ~ChatModel() override = default;

    virtual std::string model_name() const = 0;
};

} // namespace langchain::llm
