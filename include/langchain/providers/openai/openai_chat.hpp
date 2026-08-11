#pragma once

#include "langchain/llm/chat_model.hpp"

#include <optional>
#include <string>
#include <vector>

namespace langchain::providers {

struct OpenAIConfig {
    std::string model = "gpt-4o-mini";
    // If empty, read from the OPENAI_API_KEY environment variable.
    std::string api_key;
    std::string base_url = "https://api.openai.com/v1";
    double temperature = 0.7;
    std::optional<int> max_tokens;
};

// Talks to the OpenAI Chat Completions API. Also works against any
// OpenAI-compatible server (Ollama, llama.cpp server, vLLM, LM Studio, ...)
// by pointing base_url at it.
class OpenAIChat : public llm::ChatModel {
public:
    explicit OpenAIChat(OpenAIConfig config = {});

    core::Message invoke(const std::vector<core::Message>& messages) override;
    std::string model_name() const override { return config_.model; }
    std::shared_ptr<llm::ChatModel> bind_tools(std::shared_ptr<tools::ToolRegistry> registry) override;

private:
    OpenAIConfig config_;
    std::shared_ptr<tools::ToolRegistry> tools_;
};

} // namespace langchain::providers
