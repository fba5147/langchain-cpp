#pragma once

#include "langchain/rag/loaders/text_loader.hpp"

namespace langchain::rag {

// Loads a Markdown file as a single Document, same as TextLoader but
// tagged metadata["type"] = "markdown". Raw Markdown syntax is left as-is
// in content -- no stripping or rendering.
class MarkdownLoader : public TextLoader {
public:
    using TextLoader::TextLoader;

    std::vector<core::Document> load() override;
};

} // namespace langchain::rag
