#pragma once

// Shared request/response conversion for OpenAI's embeddings API shape --
// used by both OpenAIEmbeddings and AzureOpenAIEmbeddings, whose request/
// response bodies are identical (only the URL structure and auth header
// differ, same relationship as OpenAIChat/AzureOpenAIChat). Kept pure and
// separately testable, same reasoning as the chat wire-format modules.

#include <nlohmann/json.hpp>

#include <string>
#include <vector>

namespace langchain::rag::detail {

nlohmann::json build_openai_embeddings_body(const std::string& model, const std::vector<std::string>& texts);

// Reorders results by the response's own "index" field before returning
// (the API doesn't guarantee returning them in request order).
std::vector<std::vector<float>> parse_openai_embeddings_response(const nlohmann::json& response);

} // namespace langchain::rag::detail
