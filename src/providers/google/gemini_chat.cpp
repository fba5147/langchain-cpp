#include "langchain/providers/google/gemini_chat.hpp"

#include "gemini_wire_format.hpp"

#include <cpr/cpr.h>
#include <nlohmann/json.hpp>

#include <cstdlib>
#include <stdexcept>

namespace langchain::providers {

using json = nlohmann::json;

namespace {

std::string resolve_api_key(std::string configured) {
    if (!configured.empty()) {
        return configured;
    }
    if (const char* env = std::getenv("GOOGLE_API_KEY")) {
        return env;
    }
    throw std::runtime_error("GeminiChat: no api_key provided and GOOGLE_API_KEY is not set");
}

} // namespace

GeminiChat::GeminiChat(GeminiConfig config) : config_(std::move(config)) {
    config_.api_key = resolve_api_key(std::move(config_.api_key));
}

std::shared_ptr<llm::ChatModel> GeminiChat::bind_tools(std::shared_ptr<tools::ToolRegistry> registry) {
    auto bound = std::make_shared<GeminiChat>(*this);
    bound->tools_ = std::move(registry);
    return bound;
}

core::Message GeminiChat::invoke(const std::vector<core::Message>& messages) {
    detail::GeminiRequest request = detail::messages_to_gemini_request(messages);

    json generation_config{{"temperature", config_.temperature}};
    if (config_.max_output_tokens) {
        generation_config["maxOutputTokens"] = *config_.max_output_tokens;
    }

    json body{
        {"contents", request.contents},
        {"generationConfig", generation_config},
    };
    if (!request.system_instruction.empty()) {
        body["systemInstruction"] = json{{"parts", json::array({json{{"text", request.system_instruction}}})}};
    }
    if (tools_ && !tools_->all().empty()) {
        body["tools"] = tools_->to_gemini_tools_json();
    }

    std::string url = config_.base_url + "/models/" + config_.model + ":generateContent";

    cpr::Response response =
        cpr::Post(cpr::Url{url}, cpr::Header{{"x-goog-api-key", config_.api_key}, {"Content-Type", "application/json"}},
                  cpr::Body{body.dump()});

    if (response.status_code != 200) {
        throw std::runtime_error("GeminiChat: request failed (HTTP " + std::to_string(response.status_code) +
                                  "): " + response.text);
    }

    return detail::parse_gemini_message(json::parse(response.text));
}

} // namespace langchain::providers
