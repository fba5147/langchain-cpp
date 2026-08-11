#include "langchain/rag/embeddings/mock_embeddings.hpp"

#include <cctype>
#include <cmath>
#include <functional>
#include <sstream>

namespace langchain::rag {

namespace {

std::string normalize_word(const std::string& word) {
    std::string result;
    result.reserve(word.size());
    for (char c : word) {
        if (std::isalnum(static_cast<unsigned char>(c))) {
            result += static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
        }
    }
    return result;
}

} // namespace

MockEmbeddings::MockEmbeddings(std::size_t dimensions) : dimensions_(dimensions) {}

std::vector<float> MockEmbeddings::embed_query(const std::string& text) {
    std::vector<float> vector(dimensions_, 0.0f);

    std::istringstream stream(text);
    std::string word;
    std::hash<std::string> hasher;
    while (stream >> word) {
        std::string normalized = normalize_word(word);
        if (normalized.empty()) {
            continue;
        }
        std::size_t index = hasher(normalized) % dimensions_;
        vector[index] += 1.0f;
    }

    float norm = 0.0f;
    for (float value : vector) {
        norm += value * value;
    }
    norm = std::sqrt(norm);
    if (norm > 0.0f) {
        for (float& value : vector) {
            value /= norm;
        }
    }

    return vector;
}

} // namespace langchain::rag
