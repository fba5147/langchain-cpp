#include "langchain/rag/loaders/markdown_loader.hpp"
#include "langchain/rag/loaders/text_loader.hpp"

#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>
#include <stdexcept>

using namespace langchain::rag;

namespace {

std::filesystem::path write_temp_file(const std::string& name, const std::string& content) {
    auto path = std::filesystem::temp_directory_path() / name;
    std::ofstream file(path);
    file << content;
    return path;
}

} // namespace

TEST(TextLoader, LoadsFileContentAndSourceMetadata) {
    auto path = write_temp_file("langchain_cpp_text_loader_test.txt", "hello world");

    TextLoader loader(path.string());
    auto documents = loader.load();

    ASSERT_EQ(documents.size(), 1u);
    EXPECT_EQ(documents[0].content, "hello world");
    EXPECT_EQ(documents[0].metadata["source"], path.string());

    std::filesystem::remove(path);
}

TEST(TextLoader, ThrowsWhenFileMissing) {
    TextLoader loader("/no/such/file.txt");
    EXPECT_THROW(loader.load(), std::runtime_error);
}

TEST(MarkdownLoader, TagsTypeMetadata) {
    auto path = write_temp_file("langchain_cpp_markdown_loader_test.md", "# Heading");

    MarkdownLoader loader(path.string());
    auto documents = loader.load();

    ASSERT_EQ(documents.size(), 1u);
    EXPECT_EQ(documents[0].content, "# Heading");
    EXPECT_EQ(documents[0].metadata["type"], "markdown");

    std::filesystem::remove(path);
}
