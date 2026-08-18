#include "rag/embeddings/openai_embeddings_wire_format.hpp"

#include <gtest/gtest.h>

using namespace langchain::rag::detail;
using json = nlohmann::json;

TEST(BuildOpenAiEmbeddingsBody, IncludesModelAndInputArray) {
    json body = build_openai_embeddings_body("text-embedding-3-small", {"one", "two"});

    EXPECT_EQ(body["model"], "text-embedding-3-small");
    ASSERT_EQ(body["input"].size(), 2u);
    EXPECT_EQ(body["input"][0], "one");
    EXPECT_EQ(body["input"][1], "two");
}

TEST(ParseOpenAiEmbeddingsResponse, ExtractsEmbeddingsInOrder) {
    json response{{"data",
                   {{{"index", 0}, {"embedding", {0.1, 0.2}}}, {{"index", 1}, {"embedding", {0.3, 0.4}}}}}};

    auto result = parse_openai_embeddings_response(response);

    ASSERT_EQ(result.size(), 2u);
    EXPECT_FLOAT_EQ(result[0][0], 0.1f);
    EXPECT_FLOAT_EQ(result[1][0], 0.3f);
}

TEST(ParseOpenAiEmbeddingsResponse, ReordersOutOfOrderResponseByIndex) {
    json response{{"data",
                   {{{"index", 1}, {"embedding", {0.3, 0.4}}}, {{"index", 0}, {"embedding", {0.1, 0.2}}}}}};

    auto result = parse_openai_embeddings_response(response);

    ASSERT_EQ(result.size(), 2u);
    EXPECT_FLOAT_EQ(result[0][0], 0.1f);
    EXPECT_FLOAT_EQ(result[1][0], 0.3f);
}
