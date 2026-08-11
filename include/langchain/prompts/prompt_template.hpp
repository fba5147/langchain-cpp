#pragma once

#include "langchain/core/runnable.hpp"

#include <string>
#include <unordered_map>

namespace langchain::prompts {

using PromptValues = std::unordered_map<std::string, std::string>;

// Substitutes `{name}` placeholders in `template_str` from `values`.
// Placeholders with no matching key are left in the output as-is.
std::string render_template(const std::string& template_str, const PromptValues& values);

// Substitutes `{name}` placeholders in a plain string template.
class PromptTemplate : public core::Runnable<PromptValues, std::string> {
public:
    explicit PromptTemplate(std::string template_str);

    std::string format(const PromptValues& values) const;
    std::string invoke(const PromptValues& values) override;

private:
    std::string template_;
};

} // namespace langchain::prompts
