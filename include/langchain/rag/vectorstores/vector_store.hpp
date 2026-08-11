#pragma once

#include "langchain/core/document.hpp"

#include <memory>
#include <string>
#include <vector>

namespace langchain::rag {

class Retriever;

class VectorStore : public std::enable_shared_from_this<VectorStore> {
public:
    virtual ~VectorStore() = default;

    virtual void add_documents(const std::vector<core::Document>& documents) = 0;
    virtual std::vector<core::Document> similarity_search(const std::string& query, std::size_t k) = 0;

    // Wraps this store as a Runnable<string, vector<Document>>, so it
    // composes into a chain (`retriever | ...`). Must be called on a
    // VectorStore already held by a shared_ptr.
    std::shared_ptr<Retriever> as_retriever(std::size_t k = 4);
};

} // namespace langchain::rag
