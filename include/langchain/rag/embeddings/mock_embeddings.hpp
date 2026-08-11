#pragma once

#include "langchain/rag/embeddings/embeddings.hpp"

#include <cstddef>

namespace langchain::rag {

// Deterministic, hash-based bag-of-words embeddings with no network and
// no model weights: each word is lowercased, stripped of punctuation, and
// hashed into a fixed dimension, and the resulting vector is
// L2-normalized, so texts sharing more vocabulary end up closer together.
// Good enough to demo and test the retrieval pipeline offline; not a real
// semantic embedding.
class MockEmbeddings : public Embeddings {
public:
    explicit MockEmbeddings(std::size_t dimensions = 128);

    std::vector<float> embed_query(const std::string& text) override;

private:
    std::size_t dimensions_;
};

} // namespace langchain::rag
