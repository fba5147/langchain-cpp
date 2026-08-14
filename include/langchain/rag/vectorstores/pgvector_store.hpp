#pragma once

#include "langchain/rag/embeddings/embeddings.hpp"
#include "langchain/rag/vectorstores/vector_store.hpp"

#include <memory>
#include <string>

// Forward-declared so libpq's own header doesn't leak into this public
// header; PGconn is `typedef struct pg_conn PGconn` in libpq-fe.h, so
// this refers to the same type.
struct pg_conn;

namespace langchain::rag {

struct PgVectorConfig {
    // A libpq connection string, e.g. "host=localhost port=5432
    // dbname=postgres user=postgres password=...".
    std::string connection_string = "host=localhost port=5432 dbname=postgres user=postgres";
    std::string table_name = "langchain_cpp";
};

// A VectorStore backed by a real Postgres database with the pgvector
// extension. Like QdrantVectorStore, data survives this process exiting
// and can be shared across processes -- but backed by a general-purpose
// relational database rather than a dedicated vector database server.
// Requires the `vector` extension to already exist in the target
// database (`CREATE EXTENSION IF NOT EXISTS vector;`) -- this store
// doesn't create the extension itself, since that typically needs
// superuser privileges its own connection may not have. The table is
// created lazily on first add_documents() (once the embedding dimension
// is known), or reused as-is if one with the configured name already
// exists. Uses pgvector's cosine distance operator (`<=>`); doesn't
// create an approximate-nearest-neighbor index (ivfflat/hnsw) -- exact
// search only, same tradeoff as InMemoryVectorStore.
class PgVectorStore : public VectorStore {
public:
    // Throws std::runtime_error if the connection fails.
    explicit PgVectorStore(std::shared_ptr<Embeddings> embeddings, PgVectorConfig config = {});
    ~PgVectorStore() override;

    PgVectorStore(const PgVectorStore&) = delete;
    PgVectorStore& operator=(const PgVectorStore&) = delete;

    void add_documents(const std::vector<core::Document>& documents) override;
    std::vector<core::Document> similarity_search(const std::string& query, std::size_t k) override;

private:
    bool table_exists();
    void ensure_table_exists(std::size_t dimension);

    struct ConnDeleter {
        void operator()(pg_conn* connection) const;
    };

    std::shared_ptr<Embeddings> embeddings_;
    PgVectorConfig config_;
    std::unique_ptr<pg_conn, ConnDeleter> connection_;
    bool table_ready_ = false;
};

} // namespace langchain::rag
