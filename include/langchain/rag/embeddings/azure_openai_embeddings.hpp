#pragma once

#include "langchain/rag/embeddings/embeddings.hpp"

#include <string>

namespace langchain::rag {

struct AzureOpenAIEmbeddingsConfig {
    // Your Azure OpenAI deployment name (acts as the "model"). Required --
    // there's no sensible default, unlike OpenAI's model names.
    std::string deployment;
    // e.g. "https://my-resource.openai.azure.com". If empty, read from
    // AZURE_OPENAI_ENDPOINT.
    std::string endpoint;
    // If empty, read from AZURE_OPENAI_API_KEY.
    std::string api_key;
    std::string api_version = "2024-06-01";
};

// Talks to an Azure OpenAI embeddings deployment. The request/response body
// shape is identical to OpenAI's Embeddings API (shares its wire-format code
// with OpenAIEmbeddings); only the URL structure (deployment-based) and auth
// header (`api-key`, not `Authorization: Bearer`) differ.
class AzureOpenAIEmbeddings : public Embeddings {
public:
    explicit AzureOpenAIEmbeddings(AzureOpenAIEmbeddingsConfig config);

    std::vector<float> embed_query(const std::string& text) override;
    std::vector<std::vector<float>> embed_documents(const std::vector<std::string>& texts) override;

private:
    AzureOpenAIEmbeddingsConfig config_;
};

} // namespace langchain::rag
