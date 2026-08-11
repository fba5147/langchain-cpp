#include "langchain/rag/vectorstores/in_memory_vector_store.hpp"

#include "langchain/core/similarity.hpp"

#include <algorithm>
#include <numeric>

namespace langchain::rag {

InMemoryVectorStore::InMemoryVectorStore(std::shared_ptr<Embeddings> embeddings) : embeddings_(std::move(embeddings)) {}

void InMemoryVectorStore::add_documents(const std::vector<core::Document>& documents) {
    std::vector<std::string> texts;
    texts.reserve(documents.size());
    for (const auto& document : documents) {
        texts.push_back(document.content);
    }

    auto embedded = embeddings_->embed_documents(texts);

    documents_.insert(documents_.end(), documents.begin(), documents.end());
    vectors_.insert(vectors_.end(), embedded.begin(), embedded.end());
}

std::vector<core::Document> InMemoryVectorStore::similarity_search(const std::string& query, std::size_t k) {
    std::vector<float> query_vector = embeddings_->embed_query(query);

    std::vector<std::size_t> indices(documents_.size());
    std::iota(indices.begin(), indices.end(), 0);

    std::sort(indices.begin(), indices.end(), [&](std::size_t a, std::size_t b) {
        return core::cosine_similarity(query_vector, vectors_[a]) > core::cosine_similarity(query_vector, vectors_[b]);
    });

    std::size_t result_count = std::min(k, indices.size());
    std::vector<core::Document> results;
    results.reserve(result_count);
    for (std::size_t i = 0; i < result_count; ++i) {
        results.push_back(documents_[indices[i]]);
    }
    return results;
}

} // namespace langchain::rag
