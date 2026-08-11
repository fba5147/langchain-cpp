#pragma once

#include "langchain/core/message.hpp"

#include <nlohmann/json.hpp>
#include <string>
#include <vector>

// Gemini's generateContent wire format, kept separate from gemini_chat.cpp
// (and reachable from tests via a relative include, see
// tests/CMakeLists.txt) since it differs meaningfully from OpenAI's and
// Anthropic's and has no live endpoint to smoke-test against in this
// environment -- unit-testing the pure conversion logic is the next best
// thing. Internal implementation detail: lives under src/, not include/,
// and is not part of the public API.
namespace langchain::providers::detail {

struct GeminiRequest {
    nlohmann::json contents;
    std::string system_instruction; // empty if no system message was present
};

// Gemini has no call-id concept for function calling -- correlation is by
// function name alone. Message::ToolCall::id is still used internally (it
// lets AgentExecutor and this conversion agree on which tool_result
// answers which call within a single invoke() call), but is synthesized
// here rather than provided by the API, and never sent over the wire.
GeminiRequest messages_to_gemini_request(const std::vector<core::Message>& messages);

core::Message parse_gemini_message(const nlohmann::json& response_json);

} // namespace langchain::providers::detail
