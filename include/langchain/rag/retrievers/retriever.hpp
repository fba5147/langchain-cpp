#pragma once

#include "langchain/core/document.hpp"
#include "langchain/core/runnable.hpp"
#include "langchain/rag/vectorstores/vector_store.hpp"

#include <memory>
#include <string>
#include <vector>

namespace langchain::rag {

// A Runnable<string, vector<Document>> over a VectorStore's similarity
// search, so it composes directly into a chain, e.g.
// `retriever | format_context | prompt | model | parser`.
class Retriever : public core::Runnable<std::string, std::vector<core::Document>> {
public:
    Retriever(std::shared_ptr<VectorStore> store, std::size_t k = 4);

    std::vector<core::Document> invoke(const std::string& query) override;

private:
    std::shared_ptr<VectorStore> store_;
    std::size_t k_;
};

} // namespace langchain::rag
