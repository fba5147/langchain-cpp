#pragma once

#include "langchain/prompts/prompt_template.hpp"

#include <vector>

namespace langchain::prompts {

// Chooses which few-shot examples to show for a given input, rather than
// always using the same fixed set -- e.g. by semantic similarity to the
// input, so the model sees examples most relevant to what it's being
// asked right now. See SemanticSimilarityExampleSelector for a concrete
// implementation.
class ExampleSelector {
public:
    virtual ~ExampleSelector() = default;

    virtual std::vector<PromptValues> select_examples(const PromptValues& input) const = 0;
};

} // namespace langchain::prompts
