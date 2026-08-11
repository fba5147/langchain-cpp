#include "langchain/rag/vectorstores/faiss_vector_store.hpp"

#include <faiss/IndexFlat.h>

#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace langchain::rag {

namespace {

// IndexFlatIP scores by raw inner product; normalizing both indexed and
// query vectors to unit length turns that into cosine similarity, same
// as core::cosine_similarity elsewhere in the codebase.
void normalize(std::vector<float>& vector) {
    double sum_of_squares = 0.0;
    for (float value : vector) {
        sum_of_squares += static_cast<double>(value) * value;
    }
    double norm = std::sqrt(sum_of_squares);
    if (norm > 0.0) {
        for (float& value : vector) {
            value = static_cast<float>(value / norm);
        }
    }
}

} // namespace

FaissVectorStore::FaissVectorStore(std::shared_ptr<Embeddings> embeddings) : embeddings_(std::move(embeddings)) {}

FaissVectorStore::~FaissVectorStore() = default;

void FaissVectorStore::add_documents(const std::vector<core::Document>& documents) {
    if (documents.empty()) {
        return;
    }

    std::vector<std::string> texts;
    texts.reserve(documents.size());
    for (const auto& document : documents) {
        texts.push_back(document.content);
    }
    auto vectors = embeddings_->embed_documents(texts);

    if (!index_) {
        dimension_ = static_cast<int>(vectors.front().size());
        index_ = std::make_unique<faiss::IndexFlatIP>(dimension_);
    }

    std::vector<float> flat_vectors;
    flat_vectors.reserve(vectors.size() * static_cast<std::size_t>(dimension_));
    for (auto& vector : vectors) {
        if (static_cast<int>(vector.size()) != dimension_) {
            throw std::runtime_error("FaissVectorStore: all embeddings must have the same dimension");
        }
        normalize(vector);
        flat_vectors.insert(flat_vectors.end(), vector.begin(), vector.end());
    }

    index_->add(static_cast<faiss::idx_t>(vectors.size()), flat_vectors.data());
    documents_.insert(documents_.end(), documents.begin(), documents.end());
}

std::vector<core::Document> FaissVectorStore::similarity_search(const std::string& query, std::size_t k) {
    if (!index_ || documents_.empty()) {
        return {};
    }

    auto query_vector = embeddings_->embed_query(query);
    if (static_cast<int>(query_vector.size()) != dimension_) {
        throw std::runtime_error("FaissVectorStore: query embedding dimension doesn't match the index");
    }
    normalize(query_vector);

    std::size_t effective_k = std::min(k, documents_.size());
    std::vector<float> distances(effective_k);
    std::vector<faiss::idx_t> labels(effective_k);
    index_->search(1, query_vector.data(), static_cast<faiss::idx_t>(effective_k), distances.data(), labels.data());

    std::vector<core::Document> results;
    results.reserve(effective_k);
    for (std::size_t i = 0; i < effective_k; ++i) {
        if (labels[i] < 0) {
            continue; // FAISS uses -1 for "fewer than k results available"
        }
        results.push_back(documents_[static_cast<std::size_t>(labels[i])]);
    }
    return results;
}

} // namespace langchain::rag
