#include "langchain/rag/embeddings/mock_embeddings.hpp"
#include "langchain/rag/retrievers/retriever.hpp"
#include "langchain/rag/vectorstores/faiss_vector_store.hpp"

#include <gtest/gtest.h>

using namespace langchain::core;
using namespace langchain::rag;

namespace {

std::shared_ptr<FaissVectorStore> make_store_with_sample_docs() {
    auto store = std::make_shared<FaissVectorStore>(std::make_shared<MockEmbeddings>(64));
    store->add_documents({
        Document{"RAII ties resource lifetime to object lifetime in C++.", {{"topic", "raii"}}},
        Document{"Vector databases store embeddings for nearest neighbor search.", {{"topic", "vectors"}}},
        Document{"Quantum computers use superconducting qubits for computation.", {{"topic", "quantum"}}},
    });
    return store;
}

} // namespace

TEST(FaissVectorStore, SimilaritySearchReturnsMostRelevantDocumentFirst) {
    auto store = make_store_with_sample_docs();

    auto results = store->similarity_search("What is RAII in C++?", 1);

    ASSERT_EQ(results.size(), 1u);
    EXPECT_EQ(results[0].metadata["topic"], "raii");
}

TEST(FaissVectorStore, SimilaritySearchRespectsK) {
    auto store = make_store_with_sample_docs();

    EXPECT_EQ(store->similarity_search("resource lifetime", 2).size(), 2u);
    // Asking for more than exist clamps down to the corpus size rather
    // than erroring or returning FAISS's -1 "no result" padding.
    EXPECT_EQ(store->similarity_search("resource lifetime", 100).size(), 3u);
}

TEST(FaissVectorStore, EmptyStoreReturnsNoResultsRatherThanThrowing) {
    FaissVectorStore store(std::make_shared<MockEmbeddings>(64));
    EXPECT_TRUE(store.similarity_search("anything", 5).empty());
}

TEST(FaissVectorStore, AddingMultipleBatchesAccumulatesAcrossCalls) {
    auto store = std::make_shared<FaissVectorStore>(std::make_shared<MockEmbeddings>(64));
    store->add_documents({Document{"RAII ties resource lifetime to object lifetime in C++.", {{"topic", "raii"}}}});
    store->add_documents(
        {Document{"Vector databases store embeddings for nearest neighbor search.", {{"topic", "vectors"}}}});

    EXPECT_EQ(store->similarity_search("resource lifetime", 10).size(), 2u);
}

TEST(FaissVectorStore, AsRetrieverComposesIntoRunnableChain) {
    auto store = make_store_with_sample_docs();
    auto retriever = store->as_retriever(1);

    auto results = retriever->invoke("nearest neighbor search over embeddings");

    ASSERT_EQ(results.size(), 1u);
    EXPECT_EQ(results[0].metadata["topic"], "vectors");
}
