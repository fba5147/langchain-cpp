#include "langchain/core/similarity.hpp"

#include <gtest/gtest.h>

using namespace langchain::core;

TEST(CosineSimilarity, IdenticalVectorsAreOne) {
    std::vector<float> a{1.0f, 2.0f, 3.0f};
    EXPECT_NEAR(cosine_similarity(a, a), 1.0f, 1e-5f);
}

TEST(CosineSimilarity, OrthogonalVectorsAreZero) {
    std::vector<float> a{1.0f, 0.0f};
    std::vector<float> b{0.0f, 1.0f};
    EXPECT_NEAR(cosine_similarity(a, b), 0.0f, 1e-5f);
}

TEST(CosineSimilarity, OppositeVectorsAreNegativeOne) {
    std::vector<float> a{1.0f, 2.0f};
    std::vector<float> b{-1.0f, -2.0f};
    EXPECT_NEAR(cosine_similarity(a, b), -1.0f, 1e-5f);
}

TEST(CosineSimilarity, ZeroVectorReturnsZero) {
    std::vector<float> a{0.0f, 0.0f};
    std::vector<float> b{1.0f, 2.0f};
    EXPECT_EQ(cosine_similarity(a, b), 0.0f);
}
