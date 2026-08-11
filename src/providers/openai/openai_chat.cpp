#include "langchain/providers/openai/openai_chat.hpp"

#include "openai_wire_format.hpp"

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
    if (const char* env = std::getenv("OPENAI_API_KEY")) {
        return env;
    }
    throw std::runtime_error("OpenAIChat: no api_key provided and OPENAI_API_KEY is not set");
}

} // namespace

OpenAIChat::OpenAIChat(OpenAIConfig config) : config_(std::move(config)) {
    config_.api_key = resolve_api_key(std::move(config_.api_key));
}

std::shared_ptr<llm::ChatModel> OpenAIChat::bind_tools(std::shared_ptr<tools::ToolRegistry> registry) {
    auto bound = std::make_shared<OpenAIChat>(*this);
    bound->tools_ = std::move(registry);
    return bound;
}

core::Message OpenAIChat::invoke(const std::vector<core::Message>& messages) {
    json payload_messages = json::array();
    for (const auto& message : messages) {
        payload_messages.push_back(detail::message_to_openai_json(message));
    }

    json body{
        {"model", config_.model},
        {"messages", payload_messages},
        {"temperature", config_.temperature},
    };
    if (config_.max_tokens) {
        body["max_tokens"] = *config_.max_tokens;
    }
    if (tools_ && !tools_->all().empty()) {
        body["tools"] = tools_->to_openai_tools_json();
    }

    cpr::Response response = cpr::Post(cpr::Url{config_.base_url + "/chat/completions"},
                                        cpr::Header{{"Authorization", "Bearer " + config_.api_key},
                                                    {"Content-Type", "application/json"}},
                                        cpr::Body{body.dump()});

    if (response.status_code != 200) {
        throw std::runtime_error("OpenAIChat: request failed (HTTP " + std::to_string(response.status_code) +
                                  "): " + response.text);
    }

    json parsed = json::parse(response.text);
    return detail::parse_openai_message(parsed["choices"][0]["message"]);
}

} // namespace langchain::providers
