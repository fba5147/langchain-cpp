// Same reasoning as test_gemini_chat_live_contract.cpp: the pure
// request/response conversion is already unit tested
// (test_openai_embeddings_wire_format.cpp, shared with OpenAIEmbeddings)
// and the URL construction is unit tested (test_azure_url.cpp). This file
// exercises the HTTP layer itself -- the deployment-based URL, the
// `api-key` header (not `Authorization: Bearer`), and the shared body/
// response shape -- against a local mock server, since no
// AZURE_OPENAI_API_KEY is available in this environment.

#include "langchain/rag/embeddings/azure_openai_embeddings.hpp"

#include "support/mock_http_server.hpp"

#include <gtest/gtest.h>
#include <nlohmann/json.hpp>

using namespace langchain::rag;
using namespace langchain::testing;
using json = nlohmann::json;

namespace {

AzureOpenAIEmbeddingsConfig test_config(const std::string& base_url) {
    AzureOpenAIEmbeddingsConfig config;
    config.deployment = "embed-deployment";
    config.endpoint = base_url;
    config.api_key = "test-azure-key";
    return config;
}

} // namespace

TEST(AzureOpenAIEmbeddingsLiveContract, RequestHitsDocumentedUrlWithDocumentedHeaders) {
    MockHttpRequest captured;
    MockHttpServer server([&](const MockHttpRequest& request) {
        captured = request;
        return MockHttpResponse{200, json{{"data", {{{"index", 0}, {"embedding", {0.1, 0.2, 0.3}}}}}}.dump()};
    });

    AzureOpenAIEmbeddings embeddings(test_config(server.base_url()));
    auto result = embeddings.embed_query("hello world");

    EXPECT_EQ(result, (std::vector<float>{0.1f, 0.2f, 0.3f}));

    EXPECT_EQ(captured.method, "POST");
    EXPECT_EQ(captured.path, "/openai/deployments/embed-deployment/embeddings?api-version=2024-06-01");
    EXPECT_EQ(captured.header("api-key"), "test-azure-key");
    EXPECT_EQ(captured.header("Content-Type"), "application/json");

    json body = json::parse(captured.body);
    ASSERT_EQ(body["input"].size(), 1u);
    EXPECT_EQ(body["input"][0], "hello world");
}

TEST(AzureOpenAIEmbeddingsLiveContract, EmbedDocumentsBatchesIntoASingleRequestAndPreservesOrder) {
    MockHttpRequest captured;
    MockHttpServer server([&](const MockHttpRequest& request) {
        captured = request;
        return MockHttpResponse{
            200, json{{"data",
                       {{{"index", 1}, {"embedding", {0.3, 0.3}}}, {{"index", 0}, {"embedding", {0.1, 0.1}}}}}}
                     .dump()};
    });

    AzureOpenAIEmbeddings embeddings(test_config(server.base_url()));
    auto result = embeddings.embed_documents({"first", "second"});

    ASSERT_EQ(result.size(), 2u);
    EXPECT_EQ(result[0], (std::vector<float>{0.1f, 0.1f}));
    EXPECT_EQ(result[1], (std::vector<float>{0.3f, 0.3f}));

    json body = json::parse(captured.body);
    ASSERT_EQ(body["input"].size(), 2u);
    EXPECT_EQ(body["input"][0], "first");
    EXPECT_EQ(body["input"][1], "second");
}

TEST(AzureOpenAIEmbeddingsLiveContract, NonSuccessStatusThrowsWithTheServersOwnErrorBody) {
    MockHttpServer server([](const MockHttpRequest&) {
        return MockHttpResponse{401, json{{"error", {{"message", "Access denied"}}}}.dump()};
    });

    AzureOpenAIEmbeddings embeddings(test_config(server.base_url()));

    EXPECT_THROW(
        {
            try {
                embeddings.embed_query("hi");
            } catch (const std::runtime_error& error) {
                EXPECT_NE(std::string(error.what()).find("Access denied"), std::string::npos);
                throw;
            }
        },
        std::runtime_error);
}

TEST(AzureOpenAIEmbeddingsLiveContract, ThrowsWhenDeploymentIsEmpty) {
    AzureOpenAIEmbeddingsConfig config;
    config.endpoint = "https://example.openai.azure.com";
    config.api_key = "test-azure-key";

    EXPECT_THROW(AzureOpenAIEmbeddings embeddings(config), std::runtime_error);
}
