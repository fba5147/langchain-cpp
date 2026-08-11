#pragma once

#include "langchain/core/message.hpp"

#include <nlohmann/json.hpp>

// Shared Anthropic Messages API wire-format helpers. Internal
// implementation detail: lives under src/, not include/, and is not part
// of the public API. Split out (rather than kept file-local in
// anthropic_chat.cpp) so the pure conversion logic is reachable from
// tests -- there's no live endpoint to smoke-test against in this
// environment (no ANTHROPIC_API_KEY), so unit-testing this is the next
// best thing, same rationale as gemini_wire_format.hpp.
namespace langchain::providers::detail {

// Anthropic has no "tool" role: a tool result is sent back as a "user"
// message containing a tool_result content block. An assistant message
// that requests tool calls must likewise use an array of content blocks
// (text + tool_use) instead of a plain string.
//
// Throws std::runtime_error if `message.images` is non-empty --
// AnthropicChat doesn't yet encode image content into its wire format,
// so failing loudly beats silently dropping the image.
nlohmann::json message_to_anthropic_json(const core::Message& message);

core::Message parse_anthropic_message(const nlohmann::json& response_json);

} // namespace langchain::providers::detail
