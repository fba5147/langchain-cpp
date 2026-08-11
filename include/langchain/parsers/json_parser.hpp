#pragma once

#include "langchain/core/message.hpp"
#include "langchain/core/runnable.hpp"

#include <nlohmann/json.hpp>
#include <string>

namespace langchain::parsers {

// Extracts the first JSON value found in `text`. Tolerates the value being
// wrapped in a ```json ... ``` (or plain ```) fenced code block, since
// that's how most chat models format JSON when just asked for it in a
// prompt. Throws nlohmann::json::parse_error / std::runtime_error if no
// valid JSON value is found.
nlohmann::json extract_json(const std::string& text);

// Parses a Message's content as JSON.
class JsonOutputParser : public core::Runnable<core::Message, nlohmann::json> {
public:
    nlohmann::json invoke(const core::Message& message) override { return extract_json(message.content); }
};

} // namespace langchain::parsers
