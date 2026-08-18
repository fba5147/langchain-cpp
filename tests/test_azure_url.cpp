#include "providers/azure/azure_url.hpp"

#include <gtest/gtest.h>

using namespace langchain::providers::detail;

TEST(BuildAzureChatCompletionsUrl, ComposesExpectedPath) {
    auto url = build_azure_chat_completions_url("https://my-resource.openai.azure.com", "gpt4-deployment",
                                                 "2024-06-01");

    EXPECT_EQ(url,
              "https://my-resource.openai.azure.com/openai/deployments/gpt4-deployment/chat/completions"
              "?api-version=2024-06-01");
}

TEST(BuildAzureEmbeddingsUrl, ComposesExpectedPath) {
    auto url = build_azure_embeddings_url("https://my-resource.openai.azure.com", "embed-deployment", "2024-06-01");

    EXPECT_EQ(url,
              "https://my-resource.openai.azure.com/openai/deployments/embed-deployment/embeddings"
              "?api-version=2024-06-01");
}
