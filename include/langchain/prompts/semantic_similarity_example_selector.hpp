#pragma once

#include "langchain/prompts/example_selector.hpp"
#include "langchain/rag/embeddings/embeddings.hpp"

#include <memory>
#include <string>
#include <vector>

namespace langchain::prompts {

// Picks the k examples whose designated `input_key` field is most
// semantically similar to the same field of the query input, using an
// Embeddings implementation and cosine similarity. Example vectors are
// computed once at construction, not per select_examples() call.
class SemanticSimilarityExampleSelector : public ExampleSelector {
public:
    SemanticSimilarityExampleSelector(std::shared_ptr<rag::Embeddings> embeddings,
                                       std::vector<PromptValues> examples, std::string input_key,
                                       std::size_t k = 4);

    std::vector<PromptValues> select_examples(const PromptValues& input) const override;

private:
    std::shared_ptr<rag::Embeddings> embeddings_;
    std::vector<PromptValues> examples_;
    std::vector<std::vector<float>> example_vectors_;
    std::string input_key_;
    std::size_t k_;
};

} // namespace langchain::prompts
