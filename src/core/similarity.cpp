#include "langchain/core/similarity.hpp"

#include <cmath>

namespace langchain::core {

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

} // namespace langchain::core
