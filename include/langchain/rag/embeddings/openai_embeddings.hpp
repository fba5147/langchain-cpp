#pragma once

#include "langchain/rag/embeddings/embeddings.hpp"

#include <string>

namespace langchain::rag {

struct OpenAIEmbeddingsConfig {
    std::string model = "text-embedding-3-small";
    // If empty, read from the OPENAI_API_KEY environment variable.
    std::string api_key;
    std::string base_url = "https://api.openai.com/v1";
};

// Calls the OpenAI embeddings API. embed_documents batches all texts into
// a single request.
class OpenAIEmbeddings : public Embeddings {
public:
    explicit OpenAIEmbeddings(OpenAIEmbeddingsConfig config = {});

    std::vector<float> embed_query(const std::string& text) override;
    std::vector<std::vector<float>> embed_documents(const std::vector<std::string>& texts) override;

private:
    OpenAIEmbeddingsConfig config_;
};

} // namespace langchain::rag
