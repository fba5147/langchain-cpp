#pragma once

#include "langchain/core/message.hpp"

#include <nlohmann/json.hpp>

// Shared OpenAI-compatible chat wire-format helpers, used by both
// OpenAIChat and AzureOpenAIChat -- Azure's chat completions request and
// response body shape is identical to OpenAI's; only the URL and auth
// header differ. Internal implementation detail: lives under src/, not
// include/, and is not part of the public API.
namespace langchain::providers::detail {

nlohmann::json message_to_openai_json(const core::Message& message);
core::Message parse_openai_message(const nlohmann::json& message_json);

} // namespace langchain::providers::detail
