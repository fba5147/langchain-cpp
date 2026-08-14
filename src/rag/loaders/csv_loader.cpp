#include "langchain/rag/loaders/csv_loader.hpp"

#include "csv_parser.hpp"

#include <fstream>
#include <sstream>
#include <stdexcept>

namespace langchain::rag {

CsvLoader::CsvLoader(std::string path) : CsvLoader(std::move(path), Config{}) {}

CsvLoader::CsvLoader(std::string path, Config config) : path_(std::move(path)), config_(std::move(config)) {}

std::vector<core::Document> CsvLoader::load() {
    std::ifstream file(path_, std::ios::binary);
    if (!file) {
        throw std::runtime_error("CsvLoader: could not open file: " + path_);
    }
    std::ostringstream buffer;
    buffer << file.rdbuf();

    auto rows = detail::parse_csv(buffer.str(), config_.delimiter);
    std::vector<core::Document> documents;
    if (rows.empty()) {
        return documents;
    }

    const std::vector<std::string>& header = rows.front();
    int content_column_index = -1;
    if (!config_.content_column.empty()) {
        for (std::size_t i = 0; i < header.size(); ++i) {
            if (header[i] == config_.content_column) {
                content_column_index = static_cast<int>(i);
                break;
            }
        }
        if (content_column_index < 0) {
            throw std::runtime_error("CsvLoader: content_column '" + config_.content_column +
                                      "' not found in header of " + path_);
        }
    }

    documents.reserve(rows.size() - 1);
    for (std::size_t row_index = 1; row_index < rows.size(); ++row_index) {
        const std::vector<std::string>& row = rows[row_index];

        core::Document document;
        document.metadata["source"] = path_;
        document.metadata["row"] = row_index - 1;

        std::ostringstream joined_content;
        bool any_written = false;
        for (std::size_t column = 0; column < header.size() && column < row.size(); ++column) {
            document.metadata[header[column]] = row[column];
            if (static_cast<int>(column) != content_column_index) {
                if (any_written) {
                    joined_content << "\n";
                }
                joined_content << header[column] << ": " << row[column];
                any_written = true;
            }
        }

        document.content = content_column_index >= 0 && static_cast<std::size_t>(content_column_index) < row.size()
                                ? row[static_cast<std::size_t>(content_column_index)]
                                : joined_content.str();
        documents.push_back(std::move(document));
    }

    return documents;
}

} // namespace langchain::rag
