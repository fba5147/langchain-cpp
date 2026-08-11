#include "langchain/rag/format_documents.hpp"

namespace langchain::rag {

std::string FormatDocumentsAsString::invoke(const std::vector<core::Document>& documents) {
    std::string context;
    for (const auto& document : documents) {
        if (!context.empty()) {
            context += "\n\n";
        }
        context += document.content;
    }
    return context;
}

} // namespace langchain::rag
