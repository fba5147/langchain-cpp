#include "langchain/rag/embeddings/openai_embeddings.hpp"

#include <cpr/cpr.h>
#include <nlohmann/json.hpp>

#include <algorithm>
#include <cstdlib>
#include <stdexcept>

namespace langchain::rag {

using json = nlohmann::json;

namespace {

std::string resolve_api_key(std::string configured) {
    if (!configured.empty()) {
        return configured;
    }
    if (const char* env = std::getenv("OPENAI_API_KEY")) {
        return env;
    }
    throw std::runtime_error("OpenAIEmbeddings: no api_key provided and OPENAI_API_KEY is not set");
}

} // namespace

OpenAIEmbeddings::OpenAIEmbeddings(OpenAIEmbeddingsConfig config) : config_(std::move(config)) {
    config_.api_key = resolve_api_key(std::move(config_.api_key));
}

std::vector<std::vector<float>> OpenAIEmbeddings::embed_documents(const std::vector<std::string>& texts) {
    json body{{"model", config_.model}, {"input", texts}};

    cpr::Response response = cpr::Post(cpr::Url{config_.base_url + "/embeddings"},
                                        cpr::Header{{"Authorization", "Bearer " + config_.api_key},
                                                    {"Content-Type", "application/json"}},
                                        cpr::Body{body.dump()});

    if (response.status_code != 200) {
        throw std::runtime_error("OpenAIEmbeddings: request failed (HTTP " + std::to_string(response.status_code) +
                                  "): " + response.text);
    }

    json parsed = json::parse(response.text);
    std::vector<json> data(parsed["data"].begin(), parsed["data"].end());
    std::sort(data.begin(), data.end(),
              [](const json& a, const json& b) { return a["index"].get<int>() < b["index"].get<int>(); });

    std::vector<std::vector<float>> result;
    result.reserve(data.size());
    for (const auto& entry : data) {
        result.push_back(entry["embedding"].get<std::vector<float>>());
    }
    return result;
}

std::vector<float> OpenAIEmbeddings::embed_query(const std::string& text) { return embed_documents({text}).front(); }

} // namespace langchain::rag
