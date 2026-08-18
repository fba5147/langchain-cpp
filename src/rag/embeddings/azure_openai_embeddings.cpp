#include "langchain/rag/embeddings/azure_openai_embeddings.hpp"

#include "../../providers/azure/azure_url.hpp"
#include "openai_embeddings_wire_format.hpp"

#include <cpr/cpr.h>
#include <nlohmann/json.hpp>

#include <cstdlib>
#include <stdexcept>

namespace langchain::rag {

using json = nlohmann::json;

namespace {

std::string resolve(std::string configured, const char* env_name, const char* what) {
    if (!configured.empty()) {
        return configured;
    }
    if (const char* env = std::getenv(env_name)) {
        return env;
    }
    throw std::runtime_error(std::string("AzureOpenAIEmbeddings: no ") + what + " provided and " + env_name +
                              " is not set");
}

std::string strip_trailing_slash(std::string url) {
    if (!url.empty() && url.back() == '/') {
        url.pop_back();
    }
    return url;
}

} // namespace

AzureOpenAIEmbeddings::AzureOpenAIEmbeddings(AzureOpenAIEmbeddingsConfig config) : config_(std::move(config)) {
    if (config_.deployment.empty()) {
        throw std::runtime_error(
            "AzureOpenAIEmbeddings: deployment must be set to your Azure OpenAI deployment name");
    }
    config_.api_key = resolve(std::move(config_.api_key), "AZURE_OPENAI_API_KEY", "api_key");
    config_.endpoint =
        strip_trailing_slash(resolve(std::move(config_.endpoint), "AZURE_OPENAI_ENDPOINT", "endpoint"));
}

std::vector<std::vector<float>> AzureOpenAIEmbeddings::embed_documents(const std::vector<std::string>& texts) {
    json body = detail::build_openai_embeddings_body(config_.deployment, texts);

    std::string url = providers::detail::build_azure_embeddings_url(config_.endpoint, config_.deployment,
                                                                      config_.api_version);

    cpr::Response response =
        cpr::Post(cpr::Url{url}, cpr::Header{{"api-key", config_.api_key}, {"Content-Type", "application/json"}},
                  cpr::Body{body.dump()});

    if (response.status_code != 200) {
        throw std::runtime_error("AzureOpenAIEmbeddings: request failed (HTTP " +
                                  std::to_string(response.status_code) + "): " + response.text);
    }

    return detail::parse_openai_embeddings_response(json::parse(response.text));
}

std::vector<float> AzureOpenAIEmbeddings::embed_query(const std::string& text) {
    return embed_documents({text}).front();
}

} // namespace langchain::rag
