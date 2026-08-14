#pragma once

#include "langchain/rag/loaders/loader.hpp"

#include <string>

namespace langchain::rag {

// Loads a CSV file, one Document per data row (the first row is treated
// as a header). Each row's other columns become metadata (e.g.
// metadata["name"] = "Ada"), and content is every column joined as
// "column: value" lines -- a reasonable default for embedding a whole
// row's meaning. Set Config::content_column to a header name to use just
// that column's value as content instead (the rest still become
// metadata). Throws std::runtime_error if the file can't be read.
class CsvLoader : public DocumentLoader {
public:
    struct Config {
        char delimiter = ',';
        // If non-empty, must match a header name; that column's value
        // becomes content on its own instead of the joined default.
        std::string content_column;
    };

    explicit CsvLoader(std::string path);
    CsvLoader(std::string path, Config config);

    std::vector<core::Document> load() override;

private:
    std::string path_;
    Config config_;
};

} // namespace langchain::rag
