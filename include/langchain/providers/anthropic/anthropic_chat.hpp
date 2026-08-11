#pragma once

#include "langchain/llm/chat_model.hpp"

#include <string>
#include <vector>

namespace langchain::providers {

struct AnthropicConfig {
    std::string model = "claude-sonnet-5";
    // If empty, read from the ANTHROPIC_API_KEY environment variable.
    std::string api_key;
    std::string base_url = "https://api.anthropic.com/v1";
    std::string api_version = "2023-06-01";
    int max_tokens = 1024;
    double temperature = 0.7;
};

// Talks to the Anthropic Messages API.
class AnthropicChat : public llm::ChatModel {
public:
    explicit AnthropicChat(AnthropicConfig config = {});

    core::Message invoke(const std::vector<core::Message>& messages) override;
    std::string model_name() const override { return config_.model; }

private:
    AnthropicConfig config_;
};

} // namespace langchain::providers
