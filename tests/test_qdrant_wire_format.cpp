// Direct unit tests for the pure Qdrant request/response conversion
// logic -- verified by hand against a real local Qdrant server (`docker
// run -p 6333:6333 qdrant/qdrant`) while writing this, same reasoning as
// the provider wire-format modules. See
// tests/test_qdrant_vector_store_live.cpp for the live, HTTP-backed
// coverage.

#include "rag/vectorstores/qdrant_wire_format.hpp"

#include <gtest/gtest.h>

#include <regex>

using namespace langchain::core;
using namespace langchain::rag::detail;
using json = nlohmann::json;

TEST(BuildCreateCollectionBody, EncodesSizeAndCosineDistance) {
    auto body = build_create_collection_body(4);
    EXPECT_EQ(body["vectors"]["size"], 4);
    EXPECT_EQ(body["vectors"]["distance"], "Cosine");
}

TEST(BuildUpsertPointsBody, EncodesIdVectorAndPayloadPerPoint) {
    std::vector<std::string> ids{"id-1", "id-2"};
    std::vector<std::vector<float>> vectors{{1.0f, 0.0f}, {0.0f, 1.0f}};
    std::vector<Document> documents{
        Document{"first", {{"topic", "a"}}},
        Document{"second", {{"topic", "b"}}},
    };

    auto body = build_upsert_points_body(ids, vectors, documents);

    ASSERT_EQ(body["points"].size(), 2u);
    EXPECT_EQ(body["points"][0]["id"], "id-1");
    EXPECT_EQ(body["points"][0]["vector"][0], 1.0f);
    EXPECT_EQ(body["points"][0]["payload"]["content"], "first");
    EXPECT_EQ(body["points"][0]["payload"]["metadata"]["topic"], "a");
    EXPECT_EQ(body["points"][1]["id"], "id-2");
}

TEST(BuildSearchBody, EncodesVectorLimitAndWithPayload) {
    auto body = build_search_body({1.0f, 2.0f, 3.0f}, 5);
    EXPECT_EQ(body["vector"].size(), 3u);
    EXPECT_EQ(body["limit"], 5);
    EXPECT_TRUE(body["with_payload"]);
}

TEST(ParseSearchResponse, ReconstructsDocumentsInResultOrder) {
    json response{{"result",
                   {{{"id", "id-1"}, {"score", 0.9}, {"payload", {{"content", "first"}, {"metadata", {{"topic", "a"}}}}}},
                    {{"id", "id-2"}, {"score", 0.5}, {"payload", {{"content", "second"}, {"metadata", {{"topic", "b"}}}}}}}}};

    auto documents = parse_search_response(response);

    ASSERT_EQ(documents.size(), 2u);
    EXPECT_EQ(documents[0].content, "first");
    EXPECT_EQ(documents[0].metadata["topic"], "a");
    EXPECT_EQ(documents[1].content, "second");
}

TEST(ParseSearchResponse, EmptyResultReturnsNoDocuments) {
    json response{{"result", json::array()}};
    EXPECT_TRUE(parse_search_response(response).empty());
}

TEST(GenerateUuidV4, ProducesRfc4122FormattedString) {
    // 8-4-4-4-12 hex groups, version nibble '4', variant nibble in [8, b].
    static const std::regex kUuidV4Pattern(
        "^[0-9a-f]{8}-[0-9a-f]{4}-4[0-9a-f]{3}-[89ab][0-9a-f]{3}-[0-9a-f]{12}$");

    std::string uuid = generate_uuid_v4();
    EXPECT_TRUE(std::regex_match(uuid, kUuidV4Pattern)) << uuid;
}

TEST(GenerateUuidV4, ProducesDistinctValuesAcrossCalls) {
    EXPECT_NE(generate_uuid_v4(), generate_uuid_v4());
}
