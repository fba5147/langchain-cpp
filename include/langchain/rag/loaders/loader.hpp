#pragma once

#include "langchain/core/document.hpp"

#include <vector>

namespace langchain::rag {

class DocumentLoader {
public:
    virtual ~DocumentLoader() = default;

    virtual std::vector<core::Document> load() = 0;
};

} // namespace langchain::rag
