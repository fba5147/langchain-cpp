#include "langchain/parsers/json_parser.hpp"

#include <stdexcept>

namespace langchain::parsers {

namespace {

std::string strip_code_fence(const std::string& text) {
    std::size_t start = text.find("```");
    if (start == std::string::npos) {
        return text;
    }
    std::size_t content_start = text.find('\n', start);
    if (content_start == std::string::npos) {
        return text;
    }
    ++content_start;

    std::size_t end = text.find("```", content_start);
    if (end == std::string::npos) {
        return text;
    }
    return text.substr(content_start, end - content_start);
}

} // namespace

nlohmann::json extract_json(const std::string& text) {
    std::string candidate = strip_code_fence(text);

    std::size_t first = candidate.find_first_of("{[");
    std::size_t last = candidate.find_last_of("}]");
    if (first == std::string::npos || last == std::string::npos || last < first) {
        throw std::runtime_error("extract_json: no JSON object/array found in: " + text);
    }

    return nlohmann::json::parse(candidate.substr(first, last - first + 1));
}

} // namespace langchain::parsers
