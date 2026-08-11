#pragma once

#include "langchain/core/message.hpp"
#include "langchain/core/runnable.hpp"
#include "langchain/prompts/prompt_template.hpp"

#include <initializer_list>
#include <utility>
#include <vector>

namespace langchain::prompts {

// Renders a sequence of role/template pairs into a vector<Message>, ready
// to hand to a ChatModel. Output type matches ChatModel's Input type, so
// `chat_prompt | model` composes directly.
class ChatPromptTemplate : public core::Runnable<PromptValues, std::vector<core::Message>> {
public:
    // Convenience: a single templated user message, e.g.
    //   ChatPromptTemplate("Explain {topic} in one sentence.")
    explicit ChatPromptTemplate(std::string user_template);

    // Full form: a system/user/assistant conversation template, e.g.
    //   ChatPromptTemplate({
    //       {core::MessageRole::System, "You are an expert in {language}."},
    //       {core::MessageRole::User, "Explain {topic}."},
    //   })
    explicit ChatPromptTemplate(std::vector<std::pair<core::MessageRole, std::string>> templates);

    std::vector<core::Message> format(const PromptValues& values) const;
    std::vector<core::Message> invoke(const PromptValues& values) override;

private:
    std::vector<std::pair<core::MessageRole, std::string>> templates_;
};

} // namespace langchain::prompts
