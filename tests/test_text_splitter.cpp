#include "langchain/core/document.hpp"
#include "langchain/rag/splitters/recursive_character_text_splitter.hpp"

#include <gtest/gtest.h>

using namespace langchain::core;
using namespace langchain::rag;

TEST(RecursiveCharacterTextSplitter, RespectsChunkSize) {
    RecursiveCharacterTextSplitter splitter(RecursiveCharacterTextSplitter::Config{20, 0, {"\n\n", "\n", " ", ""}});
    std::string text = "This is a fairly long sentence that should be split into multiple chunks.";

    auto chunks = splitter.split_text(text);

    ASSERT_FALSE(chunks.empty());
    for (const auto& chunk : chunks) {
        EXPECT_LE(chunk.size(), 20u);
    }
}

TEST(RecursiveCharacterTextSplitter, PreservesAllWords) {
    RecursiveCharacterTextSplitter splitter(RecursiveCharacterTextSplitter::Config{10, 0, {" ", ""}});
    std::string text = "alpha beta gamma delta";

    auto chunks = splitter.split_text(text);
    std::string rejoined;
    for (const auto& chunk : chunks) {
        rejoined += chunk;
    }

    for (const char* word : {"alpha", "beta", "gamma", "delta"}) {
        EXPECT_NE(rejoined.find(word), std::string::npos);
    }
}

TEST(RecursiveCharacterTextSplitter, ShortTextProducesSingleChunk) {
    RecursiveCharacterTextSplitter splitter(RecursiveCharacterTextSplitter::Config{1000, 200, {"\n\n", "\n", " ", ""}});

    auto chunks = splitter.split_text("short text");

    ASSERT_EQ(chunks.size(), 1u);
    EXPECT_EQ(chunks[0], "short text");
}

TEST(RecursiveCharacterTextSplitter, EmptyTextProducesNoChunks) {
    RecursiveCharacterTextSplitter splitter;
    EXPECT_TRUE(splitter.split_text("").empty());
}

TEST(RecursiveCharacterTextSplitter, SplitDocumentsCarriesMetadata) {
    RecursiveCharacterTextSplitter splitter(RecursiveCharacterTextSplitter::Config{1000, 0, {"\n\n", "\n", " ", ""}});
    Document doc{"short text", {{"source", "a.txt"}}};

    auto chunks = splitter.split_documents({doc});

    ASSERT_EQ(chunks.size(), 1u);
    EXPECT_EQ(chunks[0].metadata["source"], "a.txt");
}

TEST(RecursiveCharacterTextSplitter, OverlapCarriesTrailingContextIntoNextChunk) {
    RecursiveCharacterTextSplitter splitter(RecursiveCharacterTextSplitter::Config{10, 5, {" ", ""}});
    std::string text = "one two three four five six";

    auto chunks = splitter.split_text(text);

    ASSERT_GE(chunks.size(), 2u);
    // The end of the first chunk should reappear at the start of the second.
    EXPECT_NE(chunks[1].find(chunks[0].substr(chunks[0].size() > 3 ? chunks[0].size() - 3 : 0)), std::string::npos);
}
