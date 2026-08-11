#include "langchain/rag/loaders/markdown_loader.hpp"

namespace langchain::rag {

std::vector<core::Document> MarkdownLoader::load() {
    auto documents = TextLoader::load();
    for (auto& document : documents) {
        document.metadata["type"] = "markdown";
    }
    return documents;
}

} // namespace langchain::rag
