#include "langchain/prompts/prompt_template.hpp"

namespace langchain::prompts {

std::string render_template(const std::string& template_str, const PromptValues& values) {
    std::string result;
    result.reserve(template_str.size());

    for (std::size_t i = 0; i < template_str.size();) {
        if (template_str[i] != '{') {
            result += template_str[i];
            ++i;
            continue;
        }

        std::size_t end = template_str.find('}', i);
        if (end == std::string::npos) {
            result += template_str.substr(i);
            break;
        }

        std::string key = template_str.substr(i + 1, end - i - 1);
        if (auto it = values.find(key); it != values.end()) {
            result += it->second;
        } else {
            result += template_str.substr(i, end - i + 1);
        }
        i = end + 1;
    }

    return result;
}

PromptTemplate::PromptTemplate(std::string template_str) : template_(std::move(template_str)) {}

std::string PromptTemplate::format(const PromptValues& values) const { return render_template(template_, values); }

std::string PromptTemplate::invoke(const PromptValues& values) { return format(values); }

} // namespace langchain::prompts
