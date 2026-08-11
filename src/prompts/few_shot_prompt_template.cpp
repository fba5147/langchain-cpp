#include "langchain/prompts/few_shot_prompt_template.hpp"

namespace langchain::prompts {

FewShotPromptTemplate::FewShotPromptTemplate(Config config) : config_(std::move(config)) {}

std::string FewShotPromptTemplate::format(const PromptValues& values) const {
    std::vector<PromptValues> examples =
        config_.example_selector ? config_.example_selector->select_examples(values) : config_.examples;

    std::vector<std::string> pieces;
    if (!config_.prefix.empty()) {
        pieces.push_back(config_.prefix);
    }
    for (const auto& example : examples) {
        pieces.push_back(render_template(config_.example_template, example));
    }
    pieces.push_back(render_template(config_.suffix, values));

    std::string result;
    for (std::size_t i = 0; i < pieces.size(); ++i) {
        if (i > 0) {
            result += config_.example_separator;
        }
        result += pieces[i];
    }
    return result;
}

std::string FewShotPromptTemplate::invoke(const PromptValues& values) { return format(values); }

} // namespace langchain::prompts
