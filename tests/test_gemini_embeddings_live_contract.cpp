// Same reasoning as test_gemini_chat_live_contract.cpp: the pure
// conversion logic is already unit tested
// (test_gemini_embeddings_wire_format.cpp) and cross-checked by hand
// against Google's official embeddings docs while adding this file --
// notably confirming taskType/outputDimensionality now nest under an
// "embedContentConfig" object (the older top-level placement is
// deprecated) and that each batchEmbedContents request entry repeats the
// model as "models/{model}". This file exercises the HTTP layer itself
// against a local mock server, since no GOOGLE_API_KEY is available here.

#include "langchain/rag/embeddings/gemini_embeddings.hpp"

#include "support/mock_http_server.hpp"

#include <gtest/gtest.h>
#include <nlohmann/json.hpp>

using namespace langchain::rag;
using namespace langchain::testing;
using json = nlohmann::json;

namespace {

GeminiEmbeddingsConfig test_config(const std::string& base_url) {
    GeminiEmbeddingsConfig config;
    config.api_key = "test-gemini-key";
    config.base_url = base_url + "/v1beta";
    return config;
}

} // namespace

TEST(GeminiEmbeddingsLiveContract, EmbedQueryHitsDocumentedUrlWithRetrievalQueryTaskType) {
    MockHttpRequest captured;
    MockHttpServer server([&](const MockHttpRequest& request) {
        captured = request;
        return MockHttpResponse{200, json{{"embedding", {{"values", {0.1, 0.2}}}}}.dump()};
    });

    GeminiEmbeddings embeddings(test_config(server.base_url()));
    auto result = embeddings.embed_query("hello world");

    EXPECT_EQ(result, (std::vector<float>{0.1f, 0.2f}));

    EXPECT_EQ(captured.method, "POST");
    EXPECT_EQ(captured.path, "/v1beta/models/gemini-embedding-001:embedContent");
    EXPECT_EQ(captured.header("x-goog-api-key"), "test-gemini-key");

    json body = json::parse(captured.body);
    EXPECT_EQ(body["content"]["parts"][0]["text"], "hello world");
    EXPECT_EQ(body["embedContentConfig"]["taskType"], "RETRIEVAL_QUERY");
}

TEST(GeminiEmbeddingsLiveContract, EmbedDocumentsBatchesWithRetrievalDocumentTaskType) {
    MockHttpRequest captured;
    MockHttpServer server([&](const MockHttpRequest& request) {
        captured = request;
        return MockHttpResponse{
            200, json{{"embeddings", {{{"values", {0.1, 0.1}}}, {{"values", {0.2, 0.2}}}}}}.dump()};
    });

    GeminiEmbeddings embeddings(test_config(server.base_url()));
    auto result = embeddings.embed_documents({"first", "second"});

    ASSERT_EQ(result.size(), 2u);
    EXPECT_EQ(result[0], (std::vector<float>{0.1f, 0.1f}));
    EXPECT_EQ(result[1], (std::vector<float>{0.2f, 0.2f}));

    EXPECT_EQ(captured.path, "/v1beta/models/gemini-embedding-001:batchEmbedContents");

    json body = json::parse(captured.body);
    ASSERT_EQ(body["requests"].size(), 2u);
    EXPECT_EQ(body["requests"][0]["model"], "models/gemini-embedding-001");
    EXPECT_EQ(body["requests"][0]["embedContentConfig"]["taskType"], "RETRIEVAL_DOCUMENT");
    EXPECT_EQ(body["requests"][1]["content"]["parts"][0]["text"], "second");
}

TEST(GeminiEmbeddingsLiveContract, NonSuccessStatusThrowsWithTheServersOwnErrorBody) {
    MockHttpServer server([](const MockHttpRequest&) {
        return MockHttpResponse{
            400, json{{"error", {{"code", 400}, {"message", "API key not valid"}, {"status", "INVALID_ARGUMENT"}}}}
                     .dump()};
    });

    GeminiEmbeddings embeddings(test_config(server.base_url()));

    EXPECT_THROW(
        {
            try {
                embeddings.embed_query("hi");
            } catch (const std::runtime_error& error) {
                EXPECT_NE(std::string(error.what()).find("API key not valid"), std::string::npos);
                throw;
            }
        },
        std::runtime_error);
}
