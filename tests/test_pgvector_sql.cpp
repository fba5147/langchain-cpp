// Direct unit tests for PgVectorStore's pure SQL string construction --
// verified by hand against a real local Postgres+pgvector instance
// (`docker run -p 5432:5432 -e POSTGRES_PASSWORD=postgres
// pgvector/pgvector:pg16`, then `CREATE EXTENSION vector;`) while writing
// this. See tests/test_pgvector_store_live.cpp for the live, connection-
// backed coverage.

#include "rag/vectorstores/pgvector_sql.hpp"

#include <gtest/gtest.h>

using namespace langchain::rag::detail;

TEST(FormatVectorLiteral, EncodesAsBracketedCommaSeparatedList) {
    EXPECT_EQ(format_vector_literal({1.0f, 0.0f, 0.5f}), "[1,0,0.5]");
}

TEST(FormatVectorLiteral, EmptyVectorBecomesEmptyBrackets) {
    EXPECT_EQ(format_vector_literal({}), "[]");
}

TEST(BuildCreateTableSql, IncludesTableNameAndDimension) {
    auto sql = build_create_table_sql("my_docs", 384);
    EXPECT_NE(sql.find("CREATE TABLE IF NOT EXISTS my_docs"), std::string::npos);
    EXPECT_NE(sql.find("VECTOR(384)"), std::string::npos);
}

TEST(BuildInsertSql, OneDocumentUsesThreePlaceholders) {
    auto sql = build_insert_sql("my_docs", 1);
    EXPECT_EQ(sql, "INSERT INTO my_docs (content, metadata, embedding) VALUES ($1, $2::jsonb, $3::vector)");
}

TEST(BuildInsertSql, MultipleDocumentsGetSequentialPlaceholderGroups) {
    auto sql = build_insert_sql("my_docs", 2);
    EXPECT_EQ(sql, "INSERT INTO my_docs (content, metadata, embedding) VALUES "
                   "($1, $2::jsonb, $3::vector), ($4, $5::jsonb, $6::vector)");
}

TEST(BuildSearchSql, OrdersByCosineDistanceWithLimit) {
    auto sql = build_search_sql("my_docs", 5);
    EXPECT_EQ(sql, "SELECT content, metadata FROM my_docs ORDER BY embedding <=> $1::vector LIMIT 5");
}

TEST(BuildTableExistsSql, UsesToRegclass) {
    EXPECT_EQ(build_table_exists_sql("my_docs"), "SELECT to_regclass('my_docs')");
}
