#pragma once

#include "langchain/rag/embeddings/embeddings.hpp"
#include "langchain/rag/vectorstores/vector_store.hpp"

#include <string>

namespace langchain::rag {

struct QdrantConfig {
    std::string url = "http://localhost:6333";
    std::string collection_name = "langchain_cpp";
    // If empty, no api-key header is sent (fine for a local/unauthenticated instance).
    std::string api_key;
};

// A VectorStore backed by a real Qdrant server over its REST API --
// unlike InMemoryVectorStore/FaissVectorStore, data lives on (and
// survives restarts of) a separate process, and can be shared across
// processes or machines. The collection is created lazily on the first
// add_documents() call, once the embedding dimension is known; if a
// collection with the configured name already exists (e.g. from a
// previous run), it's reused as-is rather than recreated. Uses Qdrant's
// "Cosine" distance metric, which Qdrant normalizes for internally --
// no manual vector normalization needed on this side.
class QdrantVectorStore : public VectorStore {
public:
    explicit QdrantVectorStore(std::shared_ptr<Embeddings> embeddings, QdrantConfig config = {});

    void add_documents(const std::vector<core::Document>& documents) override;
    std::vector<core::Document> similarity_search(const std::string& query, std::size_t k) override;

private:
    bool collection_exists();
    void ensure_collection_exists(std::size_t dimension);

    std::shared_ptr<Embeddings> embeddings_;
    QdrantConfig config_;
    bool collection_ready_ = false;
};

} // namespace langchain::rag
