#include "langchain/rag/vectorstores/pgvector_store.hpp"

#include "pgvector_sql.hpp"

#include <libpq-fe.h>
#include <nlohmann/json.hpp>

#include <stdexcept>

namespace langchain::rag {

using json = nlohmann::json;

void PgVectorStore::ConnDeleter::operator()(pg_conn* connection) const {
    PQfinish(reinterpret_cast<PGconn*>(connection));
}

namespace {

PGconn* as_pgconn(pg_conn* connection) { return reinterpret_cast<PGconn*>(connection); }

// Runs a parameterized query and throws with the server's own error
// message on failure; `expected_status` is PGRES_COMMAND_OK for
// statements with no result rows, PGRES_TUPLES_OK for ones that return
// rows.
PGresult* exec_checked(PGconn* connection, const std::string& sql, const std::vector<const char*>& params,
                       ExecStatusType expected_status, const std::string& what) {
    PGresult* result = PQexecParams(connection, sql.c_str(), static_cast<int>(params.size()), nullptr, params.data(),
                                     nullptr, nullptr, 0);
    if (PQresultStatus(result) != expected_status) {
        std::string message = PQerrorMessage(connection);
        PQclear(result);
        throw std::runtime_error("PgVectorStore: " + what + " failed: " + message);
    }
    return result;
}

} // namespace

PgVectorStore::PgVectorStore(std::shared_ptr<Embeddings> embeddings, PgVectorConfig config)
    : embeddings_(std::move(embeddings)), config_(std::move(config)) {
    connection_.reset(reinterpret_cast<pg_conn*>(PQconnectdb(config_.connection_string.c_str())));
    if (PQstatus(as_pgconn(connection_.get())) != CONNECTION_OK) {
        throw std::runtime_error("PgVectorStore: connection failed: " +
                                  std::string(PQerrorMessage(as_pgconn(connection_.get()))));
    }
}

PgVectorStore::~PgVectorStore() = default;

bool PgVectorStore::table_exists() {
    if (table_ready_) {
        return true;
    }
    PGresult* result = exec_checked(as_pgconn(connection_.get()), detail::build_table_exists_sql(config_.table_name),
                                     {}, PGRES_TUPLES_OK, "checking whether the table exists");
    table_ready_ = PQntuples(result) > 0 && PQgetisnull(result, 0, 0) == 0;
    PQclear(result);
    return table_ready_;
}

void PgVectorStore::ensure_table_exists(std::size_t dimension) {
    if (table_exists()) {
        return;
    }
    PGresult* result = exec_checked(as_pgconn(connection_.get()),
                                     detail::build_create_table_sql(config_.table_name, dimension), {},
                                     PGRES_COMMAND_OK, "creating the table");
    PQclear(result);
    table_ready_ = true;
}

void PgVectorStore::add_documents(const std::vector<core::Document>& documents) {
    if (documents.empty()) {
        return;
    }

    std::vector<std::string> texts;
    texts.reserve(documents.size());
    for (const auto& document : documents) {
        texts.push_back(document.content);
    }
    auto vectors = embeddings_->embed_documents(texts);

    ensure_table_exists(vectors.front().size());

    // Owns the parameter strings (content, metadata JSON, vector literal
    // per document) so the const char* pointers handed to PQexecParams
    // stay valid for the duration of the call.
    std::vector<std::string> param_storage;
    param_storage.reserve(documents.size() * 3);
    for (std::size_t i = 0; i < documents.size(); ++i) {
        param_storage.push_back(documents[i].content);
        param_storage.push_back(documents[i].metadata.dump());
        param_storage.push_back(detail::format_vector_literal(vectors[i]));
    }

    std::vector<const char*> params;
    params.reserve(param_storage.size());
    for (const auto& value : param_storage) {
        params.push_back(value.c_str());
    }

    PGresult* result = exec_checked(as_pgconn(connection_.get()),
                                     detail::build_insert_sql(config_.table_name, documents.size()), params,
                                     PGRES_COMMAND_OK, "inserting documents");
    PQclear(result);
}

std::vector<core::Document> PgVectorStore::similarity_search(const std::string& query, std::size_t k) {
    auto query_vector = embeddings_->embed_query(query);

    if (!table_exists()) {
        return {}; // nothing has ever been added to this table
    }

    std::string vector_literal = detail::format_vector_literal(query_vector);
    std::vector<const char*> params{vector_literal.c_str()};

    PGresult* result = exec_checked(as_pgconn(connection_.get()), detail::build_search_sql(config_.table_name, k),
                                     params, PGRES_TUPLES_OK, "searching");

    std::vector<core::Document> documents;
    int row_count = PQntuples(result);
    documents.reserve(static_cast<std::size_t>(row_count));
    for (int row = 0; row < row_count; ++row) {
        core::Document document;
        document.content = PQgetvalue(result, row, 0);
        document.metadata = json::parse(PQgetvalue(result, row, 1));
        documents.push_back(std::move(document));
    }
    PQclear(result);

    return documents;
}

} // namespace langchain::rag
