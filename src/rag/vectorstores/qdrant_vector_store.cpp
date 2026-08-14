#include "langchain/rag/vectorstores/qdrant_vector_store.hpp"

#include "qdrant_wire_format.hpp"

#include <cpr/cpr.h>
#include <nlohmann/json.hpp>

#include <stdexcept>

namespace langchain::rag {

using json = nlohmann::json;

namespace {

cpr::Header request_headers(const QdrantConfig& config) {
    if (config.api_key.empty()) {
        return cpr::Header{{"Content-Type", "application/json"}};
    }
    return cpr::Header{{"Content-Type", "application/json"}, {"api-key", config.api_key}};
}

} // namespace

QdrantVectorStore::QdrantVectorStore(std::shared_ptr<Embeddings> embeddings, QdrantConfig config)
    : embeddings_(std::move(embeddings)), config_(std::move(config)) {}

bool QdrantVectorStore::collection_exists() {
    if (collection_ready_) {
        return true;
    }
    cpr::Response response =
        cpr::Get(cpr::Url{config_.url + "/collections/" + config_.collection_name}, request_headers(config_));
    collection_ready_ = response.status_code == 200;
    return collection_ready_;
}

void QdrantVectorStore::ensure_collection_exists(std::size_t dimension) {
    if (collection_exists()) {
        return;
    }
    cpr::Response response = cpr::Put(cpr::Url{config_.url + "/collections/" + config_.collection_name},
                                       request_headers(config_),
                                       cpr::Body{detail::build_create_collection_body(dimension).dump()});
    if (response.status_code != 200) {
        throw std::runtime_error("QdrantVectorStore: failed to create collection (HTTP " +
                                  std::to_string(response.status_code) + "): " + response.text);
    }
    collection_ready_ = true;
}

void QdrantVectorStore::add_documents(const std::vector<core::Document>& documents) {
    if (documents.empty()) {
        return;
    }

    std::vector<std::string> texts;
    texts.reserve(documents.size());
    for (const auto& document : documents) {
        texts.push_back(document.content);
    }
    auto vectors = embeddings_->embed_documents(texts);

    ensure_collection_exists(vectors.front().size());

    std::vector<std::string> ids;
    ids.reserve(documents.size());
    for (std::size_t i = 0; i < documents.size(); ++i) {
        ids.push_back(detail::generate_uuid_v4());
    }

    cpr::Response response =
        cpr::Put(cpr::Url{config_.url + "/collections/" + config_.collection_name + "/points?wait=true"},
                 request_headers(config_), cpr::Body{detail::build_upsert_points_body(ids, vectors, documents).dump()});

    if (response.status_code != 200) {
        throw std::runtime_error("QdrantVectorStore: failed to upsert points (HTTP " +
                                  std::to_string(response.status_code) + "): " + response.text);
    }
}

std::vector<core::Document> QdrantVectorStore::similarity_search(const std::string& query, std::size_t k) {
    auto query_vector = embeddings_->embed_query(query);

    if (!collection_exists()) {
        return {}; // nothing has ever been added to this collection
    }

    cpr::Response response =
        cpr::Post(cpr::Url{config_.url + "/collections/" + config_.collection_name + "/points/search"},
                  request_headers(config_), cpr::Body{detail::build_search_body(query_vector, k).dump()});

    if (response.status_code != 200) {
        throw std::runtime_error("QdrantVectorStore: search failed (HTTP " + std::to_string(response.status_code) +
                                  "): " + response.text);
    }

    return detail::parse_search_response(json::parse(response.text));
}

} // namespace langchain::rag
