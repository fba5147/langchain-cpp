#pragma once

#include <vector>

namespace langchain::core {

// Cosine similarity between two equal-length vectors, in [-1, 1] (0 if
// either vector is all zeros). Shared by anything that ranks by vector
// similarity: InMemoryVectorStore's retrieval and
// SemanticSimilarityExampleSelector's example selection both use it.
float cosine_similarity(const std::vector<float>& a, const std::vector<float>& b);

} // namespace langchain::core
