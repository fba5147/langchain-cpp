#pragma once

#include "langchain/rag/loaders/loader.hpp"

#include <string>

namespace langchain::rag {

// Loads a PDF file as one Document per page (mirrors LangChain's
// PyPDFLoader), each tagged with metadata["source"] (the file path) and
// metadata["page"] (0-indexed). Throws std::runtime_error if the file
// can't be opened or isn't a valid PDF. Uses poppler-cpp for extraction;
// kept out of this header so including it doesn't pull poppler's own
// headers into every translation unit.
class PdfLoader : public DocumentLoader {
public:
    explicit PdfLoader(std::string path);

    std::vector<core::Document> load() override;

private:
    std::string path_;
};

} // namespace langchain::rag
