#include "pgvector_sql.hpp"

#include <sstream>

namespace langchain::rag::detail {

std::string format_vector_literal(const std::vector<float>& vector) {
    std::ostringstream stream;
    stream << '[';
    for (std::size_t i = 0; i < vector.size(); ++i) {
        if (i > 0) {
            stream << ',';
        }
        stream << vector[i];
    }
    stream << ']';
    return stream.str();
}

std::string build_create_table_sql(const std::string& table_name, std::size_t dimension) {
    std::ostringstream sql;
    sql << "CREATE TABLE IF NOT EXISTS " << table_name
        << " (id UUID PRIMARY KEY DEFAULT gen_random_uuid(), content TEXT NOT NULL, "
           "metadata JSONB NOT NULL DEFAULT '{}'::jsonb, embedding VECTOR("
        << dimension << ") NOT NULL)";
    return sql.str();
}

std::string build_insert_sql(const std::string& table_name, std::size_t document_count) {
    std::ostringstream sql;
    sql << "INSERT INTO " << table_name << " (content, metadata, embedding) VALUES ";
    for (std::size_t i = 0; i < document_count; ++i) {
        if (i > 0) {
            sql << ", ";
        }
        std::size_t base = i * 3;
        sql << "($" << (base + 1) << ", $" << (base + 2) << "::jsonb, $" << (base + 3) << "::vector)";
    }
    return sql.str();
}

std::string build_search_sql(const std::string& table_name, std::size_t k) {
    std::ostringstream sql;
    sql << "SELECT content, metadata FROM " << table_name << " ORDER BY embedding <=> $1::vector LIMIT " << k;
    return sql.str();
}

std::string build_table_exists_sql(const std::string& table_name) {
    return "SELECT to_regclass('" + table_name + "')";
}

} // namespace langchain::rag::detail
