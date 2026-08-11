#pragma once

#include "langchain/rag/embeddings/embeddings.hpp"
#include "langchain/rag/vectorstores/vector_store.hpp"

#include <memory>

namespace langchain::rag {

// A simple in-process vector store: embeds documents on add and does a
// brute-force cosine-similarity scan on query. Fine for small corpora,
// demos, and tests; swap in FAISS/Qdrant/Milvus for anything bigger.
class InMemoryVectorStore : public VectorStore {
public:
    explicit InMemoryVectorStore(std::shared_ptr<Embeddings> embeddings);

    void add_documents(const std::vector<core::Document>& documents) override;
    std::vector<core::Document> similarity_search(const std::string& query, std::size_t k) override;

private:
    std::shared_ptr<Embeddings> embeddings_;
    std::vector<core::Document> documents_;
    std::vector<std::vector<float>> vectors_;
};

} // namespace langchain::rag
