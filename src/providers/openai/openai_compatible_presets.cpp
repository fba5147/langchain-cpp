#include "langchain/providers/openai/openai_compatible_presets.hpp"

#include <cstdlib>
#include <stdexcept>

namespace langchain::providers {

namespace {

// Resolves independently of OpenAIChat's own OPENAI_API_KEY fallback: if
// (say) GROQ_API_KEY isn't set, we want a clear error naming that
// variable, not a confusing success (or wrong-provider auth failure) from
// silently falling back to an unrelated OPENAI_API_KEY.
std::string resolve(std::string configured, const char* env_name, const char* provider) {
    if (!configured.empty()) {
        return configured;
    }
    if (const char* env = std::getenv(env_name)) {
        return env;
    }
    throw std::runtime_error(std::string(provider) + ": no api_key provided and " + env_name + " is not set");
}

} // namespace

std::shared_ptr<OpenAIChat> GroqChat(GroqConfig config) {
    return std::make_shared<OpenAIChat>(OpenAIConfig{
        .model = config.model,
        .api_key = resolve(std::move(config.api_key), "GROQ_API_KEY", "GroqChat"),
        .base_url = config.base_url,
        .temperature = config.temperature,
        .max_tokens = config.max_tokens,
    });
}

std::shared_ptr<OpenAIChat> MistralChat(MistralConfig config) {
    return std::make_shared<OpenAIChat>(OpenAIConfig{
        .model = config.model,
        .api_key = resolve(std::move(config.api_key), "MISTRAL_API_KEY", "MistralChat"),
        .base_url = config.base_url,
        .temperature = config.temperature,
        .max_tokens = config.max_tokens,
    });
}

std::shared_ptr<OpenAIChat> DeepSeekChat(DeepSeekConfig config) {
    return std::make_shared<OpenAIChat>(OpenAIConfig{
        .model = config.model,
        .api_key = resolve(std::move(config.api_key), "DEEPSEEK_API_KEY", "DeepSeekChat"),
        .base_url = config.base_url,
        .temperature = config.temperature,
        .max_tokens = config.max_tokens,
    });
}

} // namespace langchain::providers
