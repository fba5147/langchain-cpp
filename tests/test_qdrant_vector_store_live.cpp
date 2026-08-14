// Live tests against a real Qdrant server, verifying QdrantVectorStore's
// full HTTP round trip (collection creation, upsert, search) against
// actual Qdrant responses -- not just the pure wire-format conversion
// functions (test_qdrant_wire_format.cpp). Start one with:
//
//   docker run -p 6333:6333 qdrant/qdrant
//
// CI doesn't start a Qdrant server, so these SKIP (not fail) when none is
// reachable at QDRANT_URL (default http://localhost:6333) -- an honest
// "not exercised," not a false "passed" or a hard failure for anyone
// building without Docker running.

#include "langchain/rag/embeddings/mock_embeddings.hpp"
#include "langchain/rag/retrievers/retriever.hpp"
#include "langchain/rag/vectorstores/qdrant_vector_store.hpp"

#include <cpr/cpr.h>
#include <gtest/gtest.h>

#include <cstdlib>

using namespace langchain::core;
using namespace langchain::rag;

namespace {

std::string qdrant_url() {
    if (const char* env = std::getenv("QDRANT_URL")) {
        return env;
    }
    return "http://localhost:6333";
}

bool qdrant_reachable() {
    cpr::Response response = cpr::Get(cpr::Url{qdrant_url() + "/collections"}, cpr::Timeout{500});
    return response.status_code == 200;
}

// A distinct collection per test avoids cross-test interference on the
// same live server; dropped before (in case a previous run left it
// behind) and after each test.
QdrantConfig test_config(const std::string& test_name) {
    QdrantConfig config;
    config.url = qdrant_url();
    config.collection_name = "langchain_cpp_test_" + test_name;
    return config;
}

void drop_collection(const QdrantConfig& config) {
    cpr::Delete(cpr::Url{config.url + "/collections/" + config.collection_name});
}

} // namespace

class QdrantVectorStoreLiveTest : public ::testing::Test {
protected:
    void SetUp() override {
        if (!qdrant_reachable()) {
            GTEST_SKIP() << "No Qdrant server reachable at " << qdrant_url()
                         << " -- run `docker run -p 6333:6333 qdrant/qdrant` to exercise this test.";
        }
    }
};

TEST_F(QdrantVectorStoreLiveTest, SimilaritySearchReturnsMostRelevantDocumentFirst) {
    auto config = test_config("basic_search");
    drop_collection(config);
    auto store = std::make_shared<QdrantVectorStore>(std::make_shared<MockEmbeddings>(64), config);

    store->add_documents({
        Document{"RAII ties resource lifetime to object lifetime in C++.", {{"topic", "raii"}}},
        Document{"Vector databases store embeddings for nearest neighbor search.", {{"topic", "vectors"}}},
        Document{"Quantum computers use superconducting qubits for computation.", {{"topic", "quantum"}}},
    });

    auto results = store->similarity_search("What is RAII in C++?", 1);

    ASSERT_EQ(results.size(), 1u);
    EXPECT_EQ(results[0].metadata["topic"], "raii");

    drop_collection(config);
}

TEST_F(QdrantVectorStoreLiveTest, SimilaritySearchRespectsK) {
    auto config = test_config("respects_k");
    drop_collection(config);
    auto store = std::make_shared<QdrantVectorStore>(std::make_shared<MockEmbeddings>(64), config);

    store->add_documents({
        Document{"RAII ties resource lifetime to object lifetime in C++.", {{"topic", "raii"}}},
        Document{"Vector databases store embeddings for nearest neighbor search.", {{"topic", "vectors"}}},
        Document{"Quantum computers use superconducting qubits for computation.", {{"topic", "quantum"}}},
    });

    EXPECT_EQ(store->similarity_search("resource lifetime", 2).size(), 2u);
    // Asking for more than exist should just return what's there, matching
    // InMemoryVectorStore/FaissVectorStore's behavior -- Qdrant's own
    // `limit` already works this way, so no client-side clamping is needed.
    EXPECT_EQ(store->similarity_search("resource lifetime", 100).size(), 3u);

    drop_collection(config);
}

TEST_F(QdrantVectorStoreLiveTest, EmptyCollectionReturnsNoResultsRatherThanThrowing) {
    auto config = test_config("empty_collection");
    drop_collection(config);
    QdrantVectorStore store(std::make_shared<MockEmbeddings>(64), config);

    EXPECT_TRUE(store.similarity_search("anything", 5).empty());
}

TEST_F(QdrantVectorStoreLiveTest, ReusesAnExistingCollectionAcrossInstances) {
    auto config = test_config("reuse_across_instances");
    drop_collection(config);

    {
        QdrantVectorStore first(std::make_shared<MockEmbeddings>(64), config);
        first.add_documents({Document{"persisted across a fresh instance", {{"topic", "persistence"}}}});
    }

    // A fresh instance pointed at the same collection name simulates a
    // process restart -- this is the whole point of a real remote store
    // over InMemoryVectorStore/FaissVectorStore.
    QdrantVectorStore second(std::make_shared<MockEmbeddings>(64), config);
    auto results = second.similarity_search("persisted across a fresh instance", 1);

    ASSERT_EQ(results.size(), 1u);
    EXPECT_EQ(results[0].metadata["topic"], "persistence");

    drop_collection(config);
}

TEST_F(QdrantVectorStoreLiveTest, AsRetrieverComposesIntoRunnableChain) {
    auto config = test_config("as_retriever");
    drop_collection(config);
    auto store = std::make_shared<QdrantVectorStore>(std::make_shared<MockEmbeddings>(64), config);
    store->add_documents({
        Document{"RAII ties resource lifetime to object lifetime in C++.", {{"topic", "raii"}}},
        Document{"Vector databases store embeddings for nearest neighbor search.", {{"topic", "vectors"}}},
    });

    auto retriever = store->as_retriever(1);
    auto results = retriever->invoke("nearest neighbor search over embeddings");

    ASSERT_EQ(results.size(), 1u);
    EXPECT_EQ(results[0].metadata["topic"], "vectors");

    drop_collection(config);
}
