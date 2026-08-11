#include "langchain/providers/azure/azure_openai_chat.hpp"

#include "../openai/openai_wire_format.hpp"
#include "azure_url.hpp"

#include <cpr/cpr.h>
#include <nlohmann/json.hpp>

#include <cstdlib>
#include <stdexcept>

namespace langchain::providers {

using json = nlohmann::json;

namespace {

std::string resolve(std::string configured, const char* env_name, const char* what) {
    if (!configured.empty()) {
        return configured;
    }
    if (const char* env = std::getenv(env_name)) {
        return env;
    }
    throw std::runtime_error(std::string("AzureOpenAIChat: no ") + what + " provided and " + env_name +
                              " is not set");
}

std::string strip_trailing_slash(std::string url) {
    if (!url.empty() && url.back() == '/') {
        url.pop_back();
    }
    return url;
}

} // namespace

AzureOpenAIChat::AzureOpenAIChat(AzureOpenAIConfig config) : config_(std::move(config)) {
    if (config_.deployment.empty()) {
        throw std::runtime_error("AzureOpenAIChat: deployment must be set to your Azure OpenAI deployment name");
    }
    config_.api_key = resolve(std::move(config_.api_key), "AZURE_OPENAI_API_KEY", "api_key");
    config_.endpoint = strip_trailing_slash(resolve(std::move(config_.endpoint), "AZURE_OPENAI_ENDPOINT", "endpoint"));
}

std::shared_ptr<llm::ChatModel> AzureOpenAIChat::bind_tools(std::shared_ptr<tools::ToolRegistry> registry) {
    auto bound = std::make_shared<AzureOpenAIChat>(*this);
    bound->tools_ = std::move(registry);
    return bound;
}

core::Message AzureOpenAIChat::invoke(const std::vector<core::Message>& messages) {
    json payload_messages = json::array();
    for (const auto& message : messages) {
        payload_messages.push_back(detail::message_to_openai_json(message));
    }

    json body{
        {"messages", payload_messages},
        {"temperature", config_.temperature},
    };
    if (config_.max_tokens) {
        body["max_tokens"] = *config_.max_tokens;
    }
    if (tools_ && !tools_->all().empty()) {
        body["tools"] = tools_->to_openai_tools_json();
    }

    std::string url = detail::build_azure_chat_completions_url(config_.endpoint, config_.deployment, config_.api_version);

    cpr::Response response =
        cpr::Post(cpr::Url{url}, cpr::Header{{"api-key", config_.api_key}, {"Content-Type", "application/json"}},
                  cpr::Body{body.dump()});

    if (response.status_code != 200) {
        throw std::runtime_error("AzureOpenAIChat: request failed (HTTP " + std::to_string(response.status_code) +
                                  "): " + response.text);
    }

    json parsed = json::parse(response.text);
    return detail::parse_openai_message(parsed["choices"][0]["message"]);
}

void AzureOpenAIChat::stream(const std::vector<core::Message>& messages, const StreamCallback& on_chunk) {
    json payload_messages = json::array();
    for (const auto& message : messages) {
        payload_messages.push_back(detail::message_to_openai_json(message));
    }

    json body{
        {"messages", payload_messages},
        {"temperature", config_.temperature},
        {"stream", true},
    };
    if (config_.max_tokens) {
        body["max_tokens"] = *config_.max_tokens;
    }
    if (tools_ && !tools_->all().empty()) {
        body["tools"] = tools_->to_openai_tools_json();
    }

    std::string url = detail::build_azure_chat_completions_url(config_.endpoint, config_.deployment, config_.api_version);

    detail::OpenAiStreamParser parser;

    cpr::Session session;
    session.SetUrl(cpr::Url{url});
    session.SetHeader(cpr::Header{{"api-key", config_.api_key}, {"Content-Type", "application/json"}});
    session.SetBody(cpr::Body{body.dump()});
    session.SetWriteCallback(cpr::WriteCallback([&](std::string_view data, intptr_t) -> bool {
        return parser.feed(data, [&](const std::string& delta) { on_chunk(llm::StreamChunk{delta, false, {}}); });
    }));

    cpr::Response response = session.Post();

    if (response.status_code != 200) {
        throw std::runtime_error("AzureOpenAIChat: streaming request failed (HTTP " +
                                  std::to_string(response.status_code) + ")");
    }

    on_chunk(llm::StreamChunk{"", true, parser.finish()});
}

} // namespace langchain::providers
