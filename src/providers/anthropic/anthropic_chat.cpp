#include "langchain/providers/anthropic/anthropic_chat.hpp"

#include "anthropic_wire_format.hpp"

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
    if (const char* env = std::getenv("ANTHROPIC_API_KEY")) {
        return env;
    }
    throw std::runtime_error("AnthropicChat: no api_key provided and ANTHROPIC_API_KEY is not set");
}

} // namespace

AnthropicChat::AnthropicChat(AnthropicConfig config) : config_(std::move(config)) {
    config_.api_key = resolve_api_key(std::move(config_.api_key));
}

std::shared_ptr<llm::ChatModel> AnthropicChat::bind_tools(std::shared_ptr<tools::ToolRegistry> registry) {
    auto bound = std::make_shared<AnthropicChat>(*this);
    bound->tools_ = std::move(registry);
    return bound;
}

core::Message AnthropicChat::invoke(const std::vector<core::Message>& messages) {
    std::string system_prompt;
    json payload_messages = json::array();
    for (const auto& message : messages) {
        if (message.role == core::MessageRole::System) {
            if (!system_prompt.empty()) {
                system_prompt += "\n";
            }
            system_prompt += message.content;
        } else {
            payload_messages.push_back(detail::message_to_anthropic_json(message));
        }
    }

    json body{
        {"model", config_.model},
        {"messages", payload_messages},
        {"max_tokens", config_.max_tokens},
        {"temperature", config_.temperature},
    };
    if (!system_prompt.empty()) {
        body["system"] = system_prompt;
    }
    if (tools_ && !tools_->all().empty()) {
        body["tools"] = tools_->to_anthropic_tools_json();
    }

    cpr::Response response = cpr::Post(cpr::Url{config_.base_url + "/messages"},
                                        cpr::Header{{"x-api-key", config_.api_key},
                                                    {"anthropic-version", config_.api_version},
                                                    {"Content-Type", "application/json"}},
                                        cpr::Body{body.dump()});

    if (response.status_code != 200) {
        throw std::runtime_error("AnthropicChat: request failed (HTTP " + std::to_string(response.status_code) +
                                  "): " + response.text);
    }

    return detail::parse_anthropic_message(json::parse(response.text));
}

} // namespace langchain::providers
