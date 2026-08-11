#include "langchain/prompts/chat_prompt_template.hpp"

namespace langchain::prompts {

ChatPromptTemplate::ChatPromptTemplate(std::string user_template) {
    templates_.emplace_back(core::MessageRole::User, std::move(user_template));
}

ChatPromptTemplate::ChatPromptTemplate(std::vector<std::pair<core::MessageRole, std::string>> templates)
    : templates_(std::move(templates)) {}

std::vector<core::Message> ChatPromptTemplate::format(const PromptValues& values) const {
    std::vector<core::Message> messages;
    messages.reserve(templates_.size());
    for (const auto& [role, template_str] : templates_) {
        messages.push_back(core::Message{role, render_template(template_str, values)});
    }
    return messages;
}

std::vector<core::Message> ChatPromptTemplate::invoke(const PromptValues& values) { return format(values); }

} // namespace langchain::prompts
