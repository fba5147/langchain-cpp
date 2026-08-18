#include "openai_embeddings_wire_format.hpp"

#include <algorithm>

namespace langchain::rag::detail {

using json = nlohmann::json;

json build_openai_embeddings_body(const std::string& model, const std::vector<std::string>& texts) {
    return json{{"model", model}, {"input", texts}};
}

std::vector<std::vector<float>> parse_openai_embeddings_response(const json& response) {
    std::vector<json> data(response.at("data").begin(), response.at("data").end());
    std::sort(data.begin(), data.end(),
              [](const json& a, const json& b) { return a["index"].get<int>() < b["index"].get<int>(); });

    std::vector<std::vector<float>> result;
    result.reserve(data.size());
    for (const auto& entry : data) {
        result.push_back(entry["embedding"].get<std::vector<float>>());
    }
    return result;
}

} // namespace langchain::rag::detail
