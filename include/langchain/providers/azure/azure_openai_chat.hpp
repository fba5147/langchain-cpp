#pragma once

#include "langchain/llm/chat_model.hpp"

#include <optional>
#include <string>

namespace langchain::providers {

struct AzureOpenAIConfig {
    // Your Azure OpenAI deployment name (acts as the "model"). Required --
    // there's no sensible default, unlike OpenAI's model names.
    std::string deployment;
    // e.g. "https://my-resource.openai.azure.com". If empty, read from
    // AZURE_OPENAI_ENDPOINT.
    std::string endpoint;
    // If empty, read from AZURE_OPENAI_API_KEY.
    std::string api_key;
    std::string api_version = "2024-06-01";
    double temperature = 0.7;
    std::optional<int> max_tokens;
};

// Talks to an Azure OpenAI deployment. The request/response body shape is
// identical to OpenAI's Chat Completions API (shares its wire-format code
// with OpenAIChat); only the URL structure (deployment-based) and auth
// header (`api-key`, not `Authorization: Bearer`) differ.
class AzureOpenAIChat : public llm::ChatModel {
public:
    explicit AzureOpenAIChat(AzureOpenAIConfig config);

    core::Message invoke(const std::vector<core::Message>& messages) override;
    std::string model_name() const override { return config_.deployment; }
    std::shared_ptr<llm::ChatModel> bind_tools(std::shared_ptr<tools::ToolRegistry> registry) override;

private:
    AzureOpenAIConfig config_;
    std::shared_ptr<tools::ToolRegistry> tools_;
};

} // namespace langchain::providers
