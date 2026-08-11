#pragma once

#include "langchain/providers/openai/openai_chat.hpp"

#include <memory>
#include <optional>
#include <string>

namespace langchain::providers {

// Thin presets over OpenAIChat for well-known OpenAI-compatible chat
// APIs: identical request/response wire format to OpenAI, just a
// different base_url, default model, and API-key environment variable.
// For anything not listed here, OpenAIChat itself works directly against
// any OpenAI-compatible endpoint -- just set OpenAIConfig::base_url.

struct GroqConfig {
    std::string model = "llama-3.3-70b-versatile";
    // If empty, read from the GROQ_API_KEY environment variable.
    std::string api_key;
    std::string base_url = "https://api.groq.com/openai/v1";
    double temperature = 0.7;
    std::optional<int> max_tokens;
};
std::shared_ptr<OpenAIChat> GroqChat(GroqConfig config = {});

struct MistralConfig {
    std::string model = "mistral-large-latest";
    // If empty, read from the MISTRAL_API_KEY environment variable.
    std::string api_key;
    std::string base_url = "https://api.mistral.ai/v1";
    double temperature = 0.7;
    std::optional<int> max_tokens;
};
std::shared_ptr<OpenAIChat> MistralChat(MistralConfig config = {});

struct DeepSeekConfig {
    std::string model = "deepseek-chat";
    // If empty, read from the DEEPSEEK_API_KEY environment variable.
    std::string api_key;
    std::string base_url = "https://api.deepseek.com/v1";
    double temperature = 0.7;
    std::optional<int> max_tokens;
};
std::shared_ptr<OpenAIChat> DeepSeekChat(DeepSeekConfig config = {});

} // namespace langchain::providers
