#include "rag/embeddings/gemini_embeddings_wire_format.hpp"

#include <gtest/gtest.h>

using namespace langchain::rag::detail;
using json = nlohmann::json;

TEST(BuildGeminiEmbedContentBody, NestsTaskTypeUnderEmbedContentConfig) {
    json body = build_gemini_embed_content_body("hello world", "RETRIEVAL_QUERY");

    EXPECT_EQ(body["content"]["parts"][0]["text"], "hello world");
    EXPECT_EQ(body["embedContentConfig"]["taskType"], "RETRIEVAL_QUERY");
    EXPECT_FALSE(body.contains("taskType"));
}

TEST(BuildGeminiBatchEmbedContentsBody, RepeatsModelPerRequestAndPreservesOrder) {
    json body = build_gemini_batch_embed_contents_body("gemini-embedding-001", {"one", "two"}, "RETRIEVAL_DOCUMENT");

    ASSERT_EQ(body["requests"].size(), 2u);
    EXPECT_EQ(body["requests"][0]["model"], "models/gemini-embedding-001");
    EXPECT_EQ(body["requests"][0]["content"]["parts"][0]["text"], "one");
    EXPECT_EQ(body["requests"][0]["embedContentConfig"]["taskType"], "RETRIEVAL_DOCUMENT");
    EXPECT_EQ(body["requests"][1]["content"]["parts"][0]["text"], "two");
}

TEST(ParseGeminiEmbedContentResponse, ExtractsValuesArray) {
    json response{{"embedding", {{"values", {0.1, 0.2, 0.3}}}}};

    auto result = parse_gemini_embed_content_response(response);

    EXPECT_EQ(result, (std::vector<float>{0.1f, 0.2f, 0.3f}));
}

TEST(ParseGeminiBatchEmbedContentsResponse, ExtractsValuesArrayPerEmbeddingInOrder) {
    json response{{"embeddings", {{{"values", {0.1, 0.2}}}, {{"values", {0.3, 0.4}}}}}};

    auto result = parse_gemini_batch_embed_contents_response(response);

    ASSERT_EQ(result.size(), 2u);
    EXPECT_EQ(result[0], (std::vector<float>{0.1f, 0.2f}));
    EXPECT_EQ(result[1], (std::vector<float>{0.3f, 0.4f}));
}
