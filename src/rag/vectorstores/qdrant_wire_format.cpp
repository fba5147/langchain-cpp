#include "qdrant_wire_format.hpp"

#include <cstdio>
#include <random>

namespace langchain::rag::detail {

using json = nlohmann::json;

json build_create_collection_body(std::size_t dimension) {
    return json{{"vectors", {{"size", dimension}, {"distance", "Cosine"}}}};
}

json build_upsert_points_body(const std::vector<std::string>& ids, const std::vector<std::vector<float>>& vectors,
                               const std::vector<core::Document>& documents) {
    json points = json::array();
    for (std::size_t i = 0; i < ids.size(); ++i) {
        points.push_back({
            {"id", ids[i]},
            {"vector", vectors[i]},
            {"payload", {{"content", documents[i].content}, {"metadata", documents[i].metadata}}},
        });
    }
    return json{{"points", points}};
}

json build_search_body(const std::vector<float>& query_vector, std::size_t k) {
    return json{{"vector", query_vector}, {"limit", k}, {"with_payload", true}};
}

std::vector<core::Document> parse_search_response(const json& response) {
    std::vector<core::Document> documents;
    for (const auto& item : response.value("result", json::array())) {
        const json& payload = item.value("payload", json::object());
        core::Document document;
        document.content = payload.value("content", "");
        document.metadata = payload.value("metadata", json::object());
        documents.push_back(std::move(document));
    }
    return documents;
}

std::string generate_uuid_v4() {
    static thread_local std::mt19937 engine{std::random_device{}()};
    std::uniform_int_distribution<int> byte_dist(0, 255);

    unsigned char bytes[16];
    for (auto& byte : bytes) {
        byte = static_cast<unsigned char>(byte_dist(engine));
    }
    bytes[6] = static_cast<unsigned char>((bytes[6] & 0x0F) | 0x40); // version 4
    bytes[8] = static_cast<unsigned char>((bytes[8] & 0x3F) | 0x80); // variant 10xx

    char buffer[37];
    std::snprintf(buffer, sizeof(buffer), "%02x%02x%02x%02x-%02x%02x-%02x%02x-%02x%02x-%02x%02x%02x%02x%02x%02x",
                  bytes[0], bytes[1], bytes[2], bytes[3], bytes[4], bytes[5], bytes[6], bytes[7], bytes[8], bytes[9],
                  bytes[10], bytes[11], bytes[12], bytes[13], bytes[14], bytes[15]);
    return std::string(buffer);
}

} // namespace langchain::rag::detail
