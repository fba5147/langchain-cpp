#include "langchain/rag/retrievers/retriever.hpp"

namespace langchain::rag {

Retriever::Retriever(std::shared_ptr<VectorStore> store, std::size_t k) : store_(std::move(store)), k_(k) {}

std::vector<core::Document> Retriever::invoke(const std::string& query) { return store_->similarity_search(query, k_); }

} // namespace langchain::rag
