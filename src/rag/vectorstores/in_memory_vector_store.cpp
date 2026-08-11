#include "langchain/rag/vectorstores/in_memory_vector_store.hpp"

#include <algorithm>
#include <cmath>
#include <numeric>

namespace langchain::rag {

namespace {

float cosine_similarity(const std::vector<float>& a, const std::vector<float>& b) {
    double dot = 0.0;
    double norm_a = 0.0;
    double norm_b = 0.0;
    for (std::size_t i = 0; i < a.size(); ++i) {
        dot += static_cast<double>(a[i]) * b[i];
        norm_a += static_cast<double>(a[i]) * a[i];
        norm_b += static_cast<double>(b[i]) * b[i];
    }
    if (norm_a == 0.0 || norm_b == 0.0) {
        return 0.0f;
    }
    return static_cast<float>(dot / (std::sqrt(norm_a) * std::sqrt(norm_b)));
}

} // namespace

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
        return cosine_similarity(query_vector, vectors_[a]) > cosine_similarity(query_vector, vectors_[b]);
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
