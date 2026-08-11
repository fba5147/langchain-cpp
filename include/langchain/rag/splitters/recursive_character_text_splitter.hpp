#pragma once

#include "langchain/core/document.hpp"

#include <cstddef>
#include <string>
#include <vector>

namespace langchain::rag {

// Splits text into chunks no larger than chunk_size, trying each
// separator in turn (paragraph breaks, then lines, then words, then raw
// characters as a last resort) and merging pieces back together up to
// chunk_size, with chunk_overlap characters of trailing context carried
// into the start of the next chunk.
class RecursiveCharacterTextSplitter {
public:
    struct Config {
        std::size_t chunk_size = 1000;
        std::size_t chunk_overlap = 200;
        std::vector<std::string> separators = {"\n\n", "\n", " ", ""};
    };

    RecursiveCharacterTextSplitter();
    explicit RecursiveCharacterTextSplitter(Config config);

    std::vector<std::string> split_text(const std::string& text) const;
    std::vector<core::Document> split_documents(const std::vector<core::Document>& documents) const;

private:
    std::vector<std::string> split(const std::string& text, std::size_t separator_index) const;
    std::vector<std::string> merge_splits(const std::vector<std::string>& pieces, const std::string& separator) const;

    Config config_;
};

} // namespace langchain::rag
