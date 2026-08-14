#include "langchain/rag/loaders/csv_loader.hpp"

#include "rag/loaders/csv_parser.hpp"

#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>
#include <stdexcept>

using namespace langchain::rag;
using namespace langchain::rag::detail;

namespace {

std::filesystem::path write_temp_file(const std::string& name, const std::string& content) {
    auto path = std::filesystem::temp_directory_path() / name;
    std::ofstream file(path, std::ios::binary);
    file << content;
    return path;
}

} // namespace

// --- parse_csv (pure, no file involved) ---

TEST(ParseCsv, SplitsSimpleRowsOnDelimiter) {
    auto rows = parse_csv("a,b,c\n1,2,3\n");
    ASSERT_EQ(rows.size(), 2u);
    EXPECT_EQ(rows[0], (std::vector<std::string>{"a", "b", "c"}));
    EXPECT_EQ(rows[1], (std::vector<std::string>{"1", "2", "3"}));
}

TEST(ParseCsv, HandlesQuotedFieldWithEmbeddedDelimiter) {
    auto rows = parse_csv("name,note\n\"Ada, Countess\",brilliant\n");
    ASSERT_EQ(rows.size(), 2u);
    EXPECT_EQ(rows[1][0], "Ada, Countess");
}

TEST(ParseCsv, HandlesEscapedQuoteInsideQuotedField) {
    auto rows = parse_csv("quote\n\"She said \"\"hello\"\"\"\n");
    ASSERT_EQ(rows.size(), 2u);
    EXPECT_EQ(rows[1][0], "She said \"hello\"");
}

TEST(ParseCsv, HandlesEmbeddedNewlineInsideQuotedField) {
    auto rows = parse_csv("text\n\"line one\nline two\"\n");
    ASSERT_EQ(rows.size(), 2u);
    EXPECT_EQ(rows[1][0], "line one\nline two");
}

TEST(ParseCsv, HandlesCrlfLineEndings) {
    auto rows = parse_csv("a,b\r\n1,2\r\n");
    ASSERT_EQ(rows.size(), 2u);
    EXPECT_EQ(rows[1], (std::vector<std::string>{"1", "2"}));
}

TEST(ParseCsv, NoTrailingNewlineStillYieldsLastRow) {
    auto rows = parse_csv("a,b\n1,2");
    ASSERT_EQ(rows.size(), 2u);
    EXPECT_EQ(rows[1], (std::vector<std::string>{"1", "2"}));
}

TEST(ParseCsv, RespectsCustomDelimiter) {
    auto rows = parse_csv("a;b\n1;2\n", ';');
    ASSERT_EQ(rows.size(), 2u);
    EXPECT_EQ(rows[1], (std::vector<std::string>{"1", "2"}));
}

TEST(ParseCsv, EmptyContentYieldsNoRows) {
    EXPECT_TRUE(parse_csv("").empty());
}

// --- CsvLoader (a real temp file end-to-end) ---

TEST(CsvLoader, DefaultJoinsAllColumnsAsContentAndSetsMetadataPerColumn) {
    auto path = write_temp_file("langchain_cpp_csv_loader_test.csv", "name,role\nAda,Mathematician\nGrace,Admiral\n");

    CsvLoader loader(path.string());
    auto documents = loader.load();

    ASSERT_EQ(documents.size(), 2u);
    EXPECT_EQ(documents[0].content, "name: Ada\nrole: Mathematician");
    EXPECT_EQ(documents[0].metadata["name"], "Ada");
    EXPECT_EQ(documents[0].metadata["role"], "Mathematician");
    EXPECT_EQ(documents[0].metadata["row"], 0);
    EXPECT_EQ(documents[0].metadata["source"], path.string());
    EXPECT_EQ(documents[1].content, "name: Grace\nrole: Admiral");
    EXPECT_EQ(documents[1].metadata["row"], 1);

    std::filesystem::remove(path);
}

TEST(CsvLoader, ContentColumnUsesJustThatColumnAsContent) {
    auto path = write_temp_file("langchain_cpp_csv_loader_content_column_test.csv",
                                 "name,bio\nAda,Wrote the first algorithm\n");

    CsvLoader::Config config;
    config.content_column = "bio";
    CsvLoader loader(path.string(), config);
    auto documents = loader.load();

    ASSERT_EQ(documents.size(), 1u);
    EXPECT_EQ(documents[0].content, "Wrote the first algorithm");
    EXPECT_EQ(documents[0].metadata["name"], "Ada");
    EXPECT_EQ(documents[0].metadata["bio"], "Wrote the first algorithm");

    std::filesystem::remove(path);
}

TEST(CsvLoader, ThrowsWhenContentColumnNotFoundInHeader) {
    auto path = write_temp_file("langchain_cpp_csv_loader_bad_column_test.csv", "name,bio\nAda,x\n");

    CsvLoader::Config config;
    config.content_column = "nonexistent";
    CsvLoader loader(path.string(), config);

    EXPECT_THROW(loader.load(), std::runtime_error);

    std::filesystem::remove(path);
}

TEST(CsvLoader, HeaderOnlyFileYieldsNoDocuments) {
    auto path = write_temp_file("langchain_cpp_csv_loader_header_only_test.csv", "name,role\n");

    CsvLoader loader(path.string());
    EXPECT_TRUE(loader.load().empty());

    std::filesystem::remove(path);
}

TEST(CsvLoader, ThrowsWhenFileMissing) {
    CsvLoader loader("/no/such/file.csv");
    EXPECT_THROW(loader.load(), std::runtime_error);
}
