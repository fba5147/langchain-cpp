#include "langchain/rag/embeddings/gemini_embeddings.hpp"

#include "gemini_embeddings_wire_format.hpp"

#include <cpr/cpr.h>
#include <nlohmann/json.hpp>

#include <cstdlib>
#include <stdexcept>

namespace langchain::rag {

using json = nlohmann::json;

namespace {

std::string resolve_api_key(std::string configured) {
    if (!configured.empty()) {
        return configured;
    }
    if (const char* env = std::getenv("GOOGLE_API_KEY")) {
        return env;
    }
    throw std::runtime_error("GeminiEmbeddings: no api_key provided and GOOGLE_API_KEY is not set");
}

} // namespace

GeminiEmbeddings::GeminiEmbeddings(GeminiEmbeddingsConfig config) : config_(std::move(config)) {
    config_.api_key = resolve_api_key(std::move(config_.api_key));
}

std::vector<float> GeminiEmbeddings::embed_query(const std::string& text) {
    json body = detail::build_gemini_embed_content_body(text, "RETRIEVAL_QUERY");

    std::string url = config_.base_url + "/models/" + config_.model + ":embedContent";

    cpr::Response response = cpr::Post(
        cpr::Url{url}, cpr::Header{{"x-goog-api-key", config_.api_key}, {"Content-Type", "application/json"}},
        cpr::Body{body.dump()});

    if (response.status_code != 200) {
        throw std::runtime_error("GeminiEmbeddings: request failed (HTTP " + std::to_string(response.status_code) +
                                  "): " + response.text);
    }

    return detail::parse_gemini_embed_content_response(json::parse(response.text));
}

std::vector<std::vector<float>> GeminiEmbeddings::embed_documents(const std::vector<std::string>& texts) {
    json body = detail::build_gemini_batch_embed_contents_body(config_.model, texts, "RETRIEVAL_DOCUMENT");

    std::string url = config_.base_url + "/models/" + config_.model + ":batchEmbedContents";

    cpr::Response response = cpr::Post(
        cpr::Url{url}, cpr::Header{{"x-goog-api-key", config_.api_key}, {"Content-Type", "application/json"}},
        cpr::Body{body.dump()});

    if (response.status_code != 200) {
        throw std::runtime_error("GeminiEmbeddings: request failed (HTTP " + std::to_string(response.status_code) +
                                  "): " + response.text);
    }

    return detail::parse_gemini_batch_embed_contents_response(json::parse(response.text));
}

} // namespace langchain::rag
