#pragma once

// Pure request/response conversion for the Qdrant REST API -- no HTTP
// here, so this is directly unit-testable without a running Qdrant
// instance, same reasoning as the provider wire-format modules. Verified
// by hand against a real local Qdrant server (see
// tests/test_qdrant_vector_store_live.cpp) while writing this.

#include "langchain/core/document.hpp"

#include <nlohmann/json.hpp>

#include <string>
#include <vector>

namespace langchain::rag::detail {

nlohmann::json build_create_collection_body(std::size_t dimension);

// `ids`, `vectors`, and `documents` must be the same length (one entry per point).
nlohmann::json build_upsert_points_body(const std::vector<std::string>& ids,
                                         const std::vector<std::vector<float>>& vectors,
                                         const std::vector<core::Document>& documents);

nlohmann::json build_search_body(const std::vector<float>& query_vector, std::size_t k);

// Reconstructs Documents from a /points/search response's "result" array,
// in the order Qdrant returned them (already sorted by score).
std::vector<core::Document> parse_search_response(const nlohmann::json& response);

// Qdrant point IDs must be either an unsigned integer or a UUID -- an
// arbitrary string is rejected. Generates a random UUIDv4 (RFC 4122),
// which needs no coordination across processes/machines, unlike an
// incrementing counter.
std::string generate_uuid_v4();

} // namespace langchain::rag::detail
