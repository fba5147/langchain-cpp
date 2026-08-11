#include "langchain/rag/vectorstores/vector_store.hpp"

#include "langchain/rag/retrievers/retriever.hpp"

namespace langchain::rag {

std::shared_ptr<Retriever> VectorStore::as_retriever(std::size_t k) {
    return std::make_shared<Retriever>(shared_from_this(), k);
}

} // namespace langchain::rag
