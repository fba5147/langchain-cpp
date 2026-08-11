#pragma once

#include "langchain/llm/chat_model.hpp"

#include <optional>
#include <string>

namespace langchain::providers {

struct GeminiConfig {
    std::string model = "gemini-2.5-flash";
    // If empty, read from the GOOGLE_API_KEY environment variable.
    std::string api_key;
    std::string base_url = "https://generativelanguage.googleapis.com/v1beta";
    double temperature = 0.7;
    std::optional<int> max_output_tokens;
};

// Talks to Google's Gemini API (generateContent). Gemini's wire format
// differs meaningfully from OpenAI/Anthropic: roles are "user"/"model"
// (not "assistant"), the system prompt is a separate top-level field, and
// tool calls/results use functionCall/functionResponse parts rather than
// OpenAI's tool_calls or Anthropic's tool_use blocks -- see
// src/providers/google/gemini_wire_format.hpp for the conversion.
class GeminiChat : public llm::ChatModel {
public:
    explicit GeminiChat(GeminiConfig config = {});

    core::Message invoke(const std::vector<core::Message>& messages) override;
    std::string model_name() const override { return config_.model; }
    std::shared_ptr<llm::ChatModel> bind_tools(std::shared_ptr<tools::ToolRegistry> registry) override;

private:
    GeminiConfig config_;
    std::shared_ptr<tools::ToolRegistry> tools_;
};

} // namespace langchain::providers
