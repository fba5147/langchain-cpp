#include "langchain/providers/openai/openai_chat.hpp"

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

json message_to_openai_json(const core::Message& message) {
    if (message.role == core::MessageRole::Tool) {
        return json{
            {"role", "tool"},
            {"tool_call_id", message.tool_call_id},
            {"content", message.content},
        };
    }

    json entry{{"role", core::to_api_role(message.role)}};

    if (!message.tool_calls.empty()) {
        json tool_calls = json::array();
        for (const auto& call : message.tool_calls) {
            tool_calls.push_back({
                {"id", call.id},
                {"type", "function"},
                {"function", {{"name", call.tool_name}, {"arguments", call.arguments.dump()}}},
            });
        }
        entry["tool_calls"] = tool_calls;
        entry["content"] = message.content.empty() ? json(nullptr) : json(message.content);
        return entry;
    }

    entry["content"] = message.content;
    return entry;
}

core::Message parse_openai_message(const json& message_json) {
    if (message_json.contains("tool_calls") && !message_json["tool_calls"].is_null()) {
        std::vector<core::ToolCall> calls;
        for (const auto& tc : message_json["tool_calls"]) {
            core::ToolCall call;
            call.id = tc["id"].get<std::string>();
            call.tool_name = tc["function"]["name"].get<std::string>();
            call.arguments = json::parse(tc["function"]["arguments"].get<std::string>());
            calls.push_back(std::move(call));
        }
        return core::Message::assistant_tool_calls(std::move(calls), message_json.value("content", ""));
    }
    return core::Message::assistant(message_json.value("content", ""));
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
        payload_messages.push_back(message_to_openai_json(message));
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
    return parse_openai_message(parsed["choices"][0]["message"]);
}

} // namespace langchain::providers
