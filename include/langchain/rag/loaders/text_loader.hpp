#pragma once

#include "langchain/rag/loaders/loader.hpp"

#include <string>

namespace langchain::rag {

// Loads a plain text file as a single Document, tagged with its path as
// metadata["source"]. Throws std::runtime_error if the file can't be read.
class TextLoader : public DocumentLoader {
public:
    explicit TextLoader(std::string path);

    std::vector<core::Document> load() override;

protected:
    std::string read_file() const;

    std::string path_;
};

} // namespace langchain::rag
