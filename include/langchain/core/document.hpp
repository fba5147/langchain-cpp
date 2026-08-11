#pragma once

#include <nlohmann/json.hpp>
#include <string>

namespace langchain::core {

// A unit of retrievable text plus arbitrary metadata, used by the future
// loader / splitter / vector-store / retriever stack (see README roadmap).
struct Document {
    std::string content;
    nlohmann::json metadata = nlohmann::json::object();
};

} // namespace langchain::core
