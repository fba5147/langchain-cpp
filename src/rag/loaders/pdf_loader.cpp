#include "langchain/rag/loaders/pdf_loader.hpp"

#include <poppler-document.h>
#include <poppler-page.h>

#include <memory>
#include <stdexcept>

namespace langchain::rag {

PdfLoader::PdfLoader(std::string path) : path_(std::move(path)) {}

std::vector<core::Document> PdfLoader::load() {
    std::unique_ptr<poppler::document> document(poppler::document::load_from_file(path_));
    if (!document) {
        throw std::runtime_error("PdfLoader: could not open file as a PDF: " + path_);
    }

    std::vector<core::Document> results;
    int page_count = document->pages();
    results.reserve(static_cast<std::size_t>(page_count));

    for (int i = 0; i < page_count; ++i) {
        std::unique_ptr<poppler::page> page(document->create_page(i));
        if (!page) {
            continue;
        }
        auto utf8_bytes = page->text().to_utf8();

        core::Document document_page;
        document_page.content.assign(utf8_bytes.begin(), utf8_bytes.end());
        document_page.metadata = {{"source", path_}, {"page", i}};
        results.push_back(std::move(document_page));
    }
    return results;
}

} // namespace langchain::rag
