#pragma once

#include "langchain/rag/embeddings/embeddings.hpp"
#include "langchain/rag/vectorstores/vector_store.hpp"

#include <memory>

namespace faiss {
struct Index;
} // namespace faiss

namespace langchain::rag {

// A VectorStore backed by FAISS's IndexFlatIP: exact nearest-neighbor
// search via inner product over L2-normalized vectors (i.e. cosine
// similarity), rather than InMemoryVectorStore's own hand-rolled
// brute-force loop -- the natural next step once a corpus outgrows a
// naive linear scan. faiss::Index is only forward-declared here so this
// header doesn't leak FAISS's own headers into every translation unit
// that includes it; see the .cpp for the real dependency.
class FaissVectorStore : public VectorStore {
public:
    explicit FaissVectorStore(std::shared_ptr<Embeddings> embeddings);
    ~FaissVectorStore() override;

    void add_documents(const std::vector<core::Document>& documents) override;
    std::vector<core::Document> similarity_search(const std::string& query, std::size_t k) override;

private:
    std::shared_ptr<Embeddings> embeddings_;
    std::vector<core::Document> documents_;
    std::unique_ptr<faiss::Index> index_;
    int dimension_ = -1;
};

} // namespace langchain::rag
