#pragma once

// Pure SQL string construction for PgVectorStore -- no database
// connection here, so this is directly unit-testable without a running
// Postgres instance, same reasoning as the provider/Qdrant wire-format
// modules. Verified by hand against a real local Postgres+pgvector
// instance (see tests/test_pgvector_store_live.cpp) while writing this.

#include <cstddef>
#include <string>
#include <vector>

namespace langchain::rag::detail {

// Formats a float vector as a pgvector literal, e.g. "[0.1,0.2,0.3]".
std::string format_vector_literal(const std::vector<float>& vector);

std::string build_create_table_sql(const std::string& table_name, std::size_t dimension);

// Returns a single multi-row INSERT statement for `document_count` rows,
// e.g. for 2: "INSERT INTO t (content, metadata, embedding) VALUES ($1,
// $2::jsonb, $3::vector), ($4, $5::jsonb, $6::vector)". The caller
// supplies 3 * document_count parameters, in (content, metadata,
// embedding) order per row.
std::string build_insert_sql(const std::string& table_name, std::size_t document_count);

std::string build_search_sql(const std::string& table_name, std::size_t k);

std::string build_table_exists_sql(const std::string& table_name);

} // namespace langchain::rag::detail
