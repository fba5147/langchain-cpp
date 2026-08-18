#pragma once

#include "langchain/rag/embeddings/embeddings.hpp"

#include <string>

namespace langchain::rag {

struct GeminiEmbeddingsConfig {
    // gemini-embedding-001 supports the taskType distinction embed_query
    // vs. embed_documents relies on below; the newer gemini-embedding-2
    // does not (it expects task instructions folded into the prompt text
    // instead), so it isn't a drop-in replacement here.
    std::string model = "gemini-embedding-001";
    // If empty, read from the GOOGLE_API_KEY environment variable.
    std::string api_key;
    std::string base_url = "https://generativelanguage.googleapis.com/v1beta";
};

// Calls Google's Gemini embeddings API. embed_query uses embedContent with
// taskType "RETRIEVAL_QUERY"; embed_documents batches all texts into a
// single batchEmbedContents request with taskType "RETRIEVAL_DOCUMENT" --
// Gemini embeds a search query differently from an indexed document, which
// is exactly the distinction the Embeddings base class exists to support.
class GeminiEmbeddings : public Embeddings {
public:
    explicit GeminiEmbeddings(GeminiEmbeddingsConfig config = {});

    std::vector<float> embed_query(const std::string& text) override;
    std::vector<std::vector<float>> embed_documents(const std::vector<std::string>& texts) override;

private:
    GeminiEmbeddingsConfig config_;
};

} // namespace langchain::rag
