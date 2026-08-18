#include "gemini_embeddings_wire_format.hpp"

namespace langchain::rag::detail {

using json = nlohmann::json;

namespace {

json content_for(const std::string& text) { return json{{"parts", json::array({json{{"text", text}}})}}; }

} // namespace

json build_gemini_embed_content_body(const std::string& text, const std::string& task_type) {
    return json{
        {"content", content_for(text)},
        {"embedContentConfig", {{"taskType", task_type}}},
    };
}

json build_gemini_batch_embed_contents_body(const std::string& model, const std::vector<std::string>& texts,
                                             const std::string& task_type) {
    json requests = json::array();
    for (const auto& text : texts) {
        requests.push_back(json{
            {"model", "models/" + model},
            {"content", content_for(text)},
            {"embedContentConfig", {{"taskType", task_type}}},
        });
    }
    return json{{"requests", requests}};
}

std::vector<float> parse_gemini_embed_content_response(const json& response) {
    return response.at("embedding").at("values").get<std::vector<float>>();
}

std::vector<std::vector<float>> parse_gemini_batch_embed_contents_response(const json& response) {
    std::vector<std::vector<float>> result;
    for (const auto& embedding : response.at("embeddings")) {
        result.push_back(embedding.at("values").get<std::vector<float>>());
    }
    return result;
}

} // namespace langchain::rag::detail
