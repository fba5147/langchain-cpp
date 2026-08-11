#pragma once

#include "langchain/core/message.hpp"
#include "langchain/core/runnable.hpp"
#include "langchain/llm/chat_model.hpp"

#include <memory>
#include <string>

namespace langchain::parsers {

// Wraps a parser (e.g. StructuredOutputParser<T>) so that if parsing the
// model's reply fails, it re-prompts the *same* model with the invalid
// output and the parse error, asking it to fix the formatting, and
// retries -- up to max_retries times before giving up and rethrowing the
// last parse error. Mirrors LangChain's OutputFixingParser.
template <typename T>
class OutputFixingParser : public core::Runnable<core::Message, T> {
public:
    OutputFixingParser(std::shared_ptr<core::Runnable<core::Message, T>> inner,
                        std::shared_ptr<llm::ChatModel> model, int max_retries = 1)
        : inner_(std::move(inner)), model_(std::move(model)), max_retries_(max_retries) {}

    T invoke(const core::Message& message) override {
        core::Message current = message;

        for (int attempt = 0;; ++attempt) {
            try {
                return inner_->invoke(current);
            } catch (const std::exception& e) {
                if (attempt >= max_retries_) {
                    throw;
                }
                std::string fix_prompt = "The following output could not be parsed:\n\n" + current.content +
                                          "\n\nParsing failed with this error: " + e.what() +
                                          "\n\nPlease respond again with correctly formatted output only, no "
                                          "explanation.";
                current = model_->invoke({core::Message::user(fix_prompt)});
            }
        }
    }

private:
    std::shared_ptr<core::Runnable<core::Message, T>> inner_;
    std::shared_ptr<llm::ChatModel> model_;
    int max_retries_;
};

} // namespace langchain::parsers
