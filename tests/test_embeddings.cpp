#include "langchain/rag/embeddings/mock_embeddings.hpp"

#include <cmath>
#include <gtest/gtest.h>

using namespace langchain::rag;

TEST(MockEmbeddings, SameTextProducesSameVector) {
    MockEmbeddings embeddings(16);
    EXPECT_EQ(embeddings.embed_query("hello world"), embeddings.embed_query("hello world"));
}

TEST(MockEmbeddings, DifferentTextProducesDifferentVector) {
    MockEmbeddings embeddings(16);
    EXPECT_NE(embeddings.embed_query("hello world"), embeddings.embed_query("goodbye moon"));
}

TEST(MockEmbeddings, VectorIsUnitLength) {
    MockEmbeddings embeddings(16);
    auto vector = embeddings.embed_query("some reasonably long piece of text");

    float norm_squared = 0.0f;
    for (float value : vector) {
        norm_squared += value * value;
    }
    EXPECT_NEAR(std::sqrt(norm_squared), 1.0f, 1e-4f);
}

TEST(MockEmbeddings, SharedVocabularyIsMoreSimilarThanUnrelatedText) {
    MockEmbeddings embeddings(64);
    auto a = embeddings.embed_query("cats and dogs are common household pets");
    auto b = embeddings.embed_query("dogs and cats make common household pets");
    auto c = embeddings.embed_query("quantum computers use superconducting qubits");

    auto dot = [](const std::vector<float>& x, const std::vector<float>& y) {
        float sum = 0.0f;
        for (std::size_t i = 0; i < x.size(); ++i) {
            sum += x[i] * y[i];
        }
        return sum;
    };

    EXPECT_GT(dot(a, b), dot(a, c));
}

TEST(MockEmbeddings, PunctuationAndCaseAreNormalizedAway) {
    MockEmbeddings embeddings(32);
    EXPECT_EQ(embeddings.embed_query("RAII"), embeddings.embed_query("RAII?"));
    EXPECT_EQ(embeddings.embed_query("Hello"), embeddings.embed_query("hello."));
}

TEST(MockEmbeddings, EmbedDocumentsMatchesEmbedQueryPerText) {
    MockEmbeddings embeddings(16);
    auto docs = embeddings.embed_documents({"one", "two"});

    EXPECT_EQ(docs[0], embeddings.embed_query("one"));
    EXPECT_EQ(docs[1], embeddings.embed_query("two"));
}
