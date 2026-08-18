#pragma once

// Request/response conversion for Gemini's embeddings API
// (embedContent/batchEmbedContents). Kept pure and separately testable,
// same reasoning as gemini_wire_format.hpp for chat.
//
// Verified against Google's current docs (ai.google.dev/api/embeddings)
// while writing this: taskType/outputDimensionality now live nested under
// an "embedContentConfig" object -- the older top-level placement is
// deprecated. Each entry in batchEmbedContents' "requests" array must
// repeat the model as "models/{model}", matching the top-level model,
// even though the URL already names it.

#include <nlohmann/json.hpp>

#include <string>
#include <vector>

namespace langchain::rag::detail {

// "RETRIEVAL_QUERY" or "RETRIEVAL_DOCUMENT" -- see embed_query/embed_documents
// in gemini_embeddings.cpp for which is used where.
nlohmann::json build_gemini_embed_content_body(const std::string& text, const std::string& task_type);

nlohmann::json build_gemini_batch_embed_contents_body(const std::string& model, const std::vector<std::string>& texts,
                                                        const std::string& task_type);

std::vector<float> parse_gemini_embed_content_response(const nlohmann::json& response);

std::vector<std::vector<float>> parse_gemini_batch_embed_contents_response(const nlohmann::json& response);

} // namespace langchain::rag::detail
