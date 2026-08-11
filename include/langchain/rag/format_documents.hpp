#pragma once

#include "langchain/core/document.hpp"
#include "langchain/core/runnable.hpp"

#include <string>
#include <vector>

namespace langchain::rag {

// Joins retrieved Documents' content into a single string (blank-line
// separated), for feeding into a prompt's context variable. A
// Runnable<vector<Document>, string>, so it composes directly after a
// Retriever: `retriever | format_documents`.
class FormatDocumentsAsString : public core::Runnable<std::vector<core::Document>, std::string> {
public:
    std::string invoke(const std::vector<core::Document>& documents) override;
};

} // namespace langchain::rag
