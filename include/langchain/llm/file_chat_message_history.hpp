#pragma once

#include "langchain/llm/chat_message_history.hpp"

#include <string>

namespace langchain::llm {

// Persists messages to a JSON file, surviving process restarts. Loads
// existing content on construction if the file exists (a missing file is
// not an error -- that's just a fresh conversation); rewrites the whole
// file on every add_message()/clear(). Fine for the message counts a
// single conversation accumulates; not designed for high-frequency or
// concurrent writers.
class FileChatMessageHistory : public ChatMessageHistory {
public:
    explicit FileChatMessageHistory(std::string path);

    std::vector<core::Message> messages() const override;
    void add_message(const core::Message& message) override;
    void clear() override;

private:
    void load();
    void save() const;

    std::string path_;
    std::vector<core::Message> messages_;
};

} // namespace langchain::llm
