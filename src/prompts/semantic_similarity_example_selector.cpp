#include "langchain/prompts/semantic_similarity_example_selector.hpp"

#include "langchain/core/similarity.hpp"

#include <algorithm>
#include <numeric>

namespace langchain::prompts {

namespace {

std::string field_or_empty(const PromptValues& values, const std::string& key) {
    auto it = values.find(key);
    return it != values.end() ? it->second : "";
}

} // namespace

SemanticSimilarityExampleSelector::SemanticSimilarityExampleSelector(std::shared_ptr<rag::Embeddings> embeddings,
                                                                       std::vector<PromptValues> examples,
                                                                       std::string input_key, std::size_t k)
    : embeddings_(std::move(embeddings)), examples_(std::move(examples)), input_key_(std::move(input_key)), k_(k) {
    std::vector<std::string> texts;
    texts.reserve(examples_.size());
    for (const auto& example : examples_) {
        texts.push_back(field_or_empty(example, input_key_));
    }
    example_vectors_ = embeddings_->embed_documents(texts);
}

std::vector<PromptValues> SemanticSimilarityExampleSelector::select_examples(const PromptValues& input) const {
    std::vector<float> query_vector = embeddings_->embed_query(field_or_empty(input, input_key_));

    std::vector<std::size_t> indices(examples_.size());
    std::iota(indices.begin(), indices.end(), 0);
    std::sort(indices.begin(), indices.end(), [&](std::size_t a, std::size_t b) {
        return core::cosine_similarity(query_vector, example_vectors_[a]) >
               core::cosine_similarity(query_vector, example_vectors_[b]);
    });

    std::size_t count = std::min(k_, indices.size());
    std::vector<PromptValues> result;
    result.reserve(count);
    for (std::size_t i = 0; i < count; ++i) {
        result.push_back(examples_[indices[i]]);
    }
    return result;
}

} // namespace langchain::prompts
