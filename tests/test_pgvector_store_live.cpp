// Live tests against a real Postgres+pgvector instance, verifying
// PgVectorStore's full round trip (table creation, insert, cosine-
// distance search) against actual query results -- not just the pure SQL
// string construction (test_pgvector_sql.cpp). Start one with:
//
//   docker run -p 5432:5432 -e POSTGRES_PASSWORD=postgres pgvector/pgvector:pg16
//   psql -h localhost -U postgres -c "CREATE EXTENSION IF NOT EXISTS vector;"
//
// CI doesn't start a Postgres server, so these SKIP (not fail) when none
// is reachable at PGVECTOR_TEST_CONNECTION_STRING (default below) -- an
// honest "not exercised," not a false "passed" or a hard failure for
// anyone building without Docker running.

#include "langchain/rag/embeddings/mock_embeddings.hpp"
#include "langchain/rag/retrievers/retriever.hpp"
#include "langchain/rag/vectorstores/pgvector_store.hpp"

#include <gtest/gtest.h>

#include <cstdlib>
#include <libpq-fe.h>

using namespace langchain::core;
using namespace langchain::rag;

namespace {

std::string connection_string() {
    if (const char* env = std::getenv("PGVECTOR_TEST_CONNECTION_STRING")) {
        return env;
    }
    return "host=localhost port=5432 dbname=postgres user=postgres password=postgres connect_timeout=2";
}

bool postgres_reachable() {
    PGconn* connection = PQconnectdb(connection_string().c_str());
    bool ok = PQstatus(connection) == CONNECTION_OK;
    PQfinish(connection);
    return ok;
}

void drop_table(const std::string& table_name) {
    PGconn* connection = PQconnectdb(connection_string().c_str());
    if (PQstatus(connection) == CONNECTION_OK) {
        PGresult* result = PQexec(connection, ("DROP TABLE IF EXISTS " + table_name).c_str());
        PQclear(result);
    }
    PQfinish(connection);
}

PgVectorConfig test_config(const std::string& test_name) {
    PgVectorConfig config;
    config.connection_string = connection_string();
    config.table_name = "langchain_cpp_test_" + test_name;
    return config;
}

} // namespace

class PgVectorStoreLiveTest : public ::testing::Test {
protected:
    void SetUp() override {
        if (!postgres_reachable()) {
            GTEST_SKIP() << "No Postgres server reachable -- run `docker run -p 5432:5432 -e "
                            "POSTGRES_PASSWORD=postgres pgvector/pgvector:pg16` (and `CREATE EXTENSION "
                            "vector;`) to exercise this test.";
        }
    }
};

TEST_F(PgVectorStoreLiveTest, SimilaritySearchReturnsMostRelevantDocumentFirst) {
    auto config = test_config("basic_search");
    drop_table(config.table_name);
    auto store = std::make_shared<PgVectorStore>(std::make_shared<MockEmbeddings>(64), config);

    store->add_documents({
        Document{"RAII ties resource lifetime to object lifetime in C++.", {{"topic", "raii"}}},
        Document{"Vector databases store embeddings for nearest neighbor search.", {{"topic", "vectors"}}},
        Document{"Quantum computers use superconducting qubits for computation.", {{"topic", "quantum"}}},
    });

    auto results = store->similarity_search("What is RAII in C++?", 1);

    ASSERT_EQ(results.size(), 1u);
    EXPECT_EQ(results[0].metadata["topic"], "raii");

    drop_table(config.table_name);
}

TEST_F(PgVectorStoreLiveTest, SimilaritySearchRespectsK) {
    auto config = test_config("respects_k");
    drop_table(config.table_name);
    auto store = std::make_shared<PgVectorStore>(std::make_shared<MockEmbeddings>(64), config);

    store->add_documents({
        Document{"RAII ties resource lifetime to object lifetime in C++.", {{"topic", "raii"}}},
        Document{"Vector databases store embeddings for nearest neighbor search.", {{"topic", "vectors"}}},
        Document{"Quantum computers use superconducting qubits for computation.", {{"topic", "quantum"}}},
    });

    EXPECT_EQ(store->similarity_search("resource lifetime", 2).size(), 2u);
    EXPECT_EQ(store->similarity_search("resource lifetime", 100).size(), 3u);

    drop_table(config.table_name);
}

TEST_F(PgVectorStoreLiveTest, EmptyTableReturnsNoResultsRatherThanThrowing) {
    auto config = test_config("empty_table");
    drop_table(config.table_name);
    PgVectorStore store(std::make_shared<MockEmbeddings>(64), config);

    EXPECT_TRUE(store.similarity_search("anything", 5).empty());
}

TEST_F(PgVectorStoreLiveTest, ReusesAnExistingTableAcrossInstances) {
    auto config = test_config("reuse_across_instances");
    drop_table(config.table_name);

    {
        PgVectorStore first(std::make_shared<MockEmbeddings>(64), config);
        first.add_documents({Document{"persisted across a fresh instance", {{"topic", "persistence"}}}});
    }

    // A fresh instance (and connection) pointed at the same table name
    // simulates a process restart -- the whole point of a real remote
    // store over InMemoryVectorStore/FaissVectorStore.
    PgVectorStore second(std::make_shared<MockEmbeddings>(64), config);
    auto results = second.similarity_search("persisted across a fresh instance", 1);

    ASSERT_EQ(results.size(), 1u);
    EXPECT_EQ(results[0].metadata["topic"], "persistence");

    drop_table(config.table_name);
}

TEST_F(PgVectorStoreLiveTest, AsRetrieverComposesIntoRunnableChain) {
    auto config = test_config("as_retriever");
    drop_table(config.table_name);
    auto store = std::make_shared<PgVectorStore>(std::make_shared<MockEmbeddings>(64), config);
    store->add_documents({
        Document{"RAII ties resource lifetime to object lifetime in C++.", {{"topic", "raii"}}},
        Document{"Vector databases store embeddings for nearest neighbor search.", {{"topic", "vectors"}}},
    });

    auto retriever = store->as_retriever(1);
    auto results = retriever->invoke("nearest neighbor search over embeddings");

    ASSERT_EQ(results.size(), 1u);
    EXPECT_EQ(results[0].metadata["topic"], "vectors");

    drop_table(config.table_name);
}
