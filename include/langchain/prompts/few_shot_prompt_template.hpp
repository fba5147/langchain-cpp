#pragma once

#include "langchain/core/runnable.hpp"
#include "langchain/prompts/example_selector.hpp"
#include "langchain/prompts/prompt_template.hpp"

#include <memory>
#include <string>
#include <vector>

namespace langchain::prompts {

// The classic few-shot prompting pattern: a fixed prefix, then each
// example rendered through `example_template` (joined by
// `example_separator`), then a suffix rendered with the caller's own
// values -- showing the model a few worked examples before asking it to
// do the same thing for a new input.
class FewShotPromptTemplate : public core::Runnable<PromptValues, std::string> {
public:
    struct Config {
        std::string prefix;
        std::string example_template; // rendered once per example
        std::string suffix;           // rendered with the caller's own PromptValues
        // Fixed examples, used when example_selector is null.
        std::vector<PromptValues> examples;
        // If set, takes precedence over `examples`: examples are chosen
        // per-input (e.g. by semantic similarity) instead of always being
        // the same fixed set.
        std::shared_ptr<ExampleSelector> example_selector;
        std::string example_separator = "\n\n";
    };

    explicit FewShotPromptTemplate(Config config);

    std::string format(const PromptValues& values) const;
    std::string invoke(const PromptValues& values) override;

private:
    Config config_;
};

} // namespace langchain::prompts
