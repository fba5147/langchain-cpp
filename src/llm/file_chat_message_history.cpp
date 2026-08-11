#include "langchain/llm/file_chat_message_history.hpp"

#include <nlohmann/json.hpp>

#include <fstream>
#include <stdexcept>

namespace langchain::llm {

using json = nlohmann::json;

namespace {

core::MessageRole parse_role(const std::string& role) {
    if (role == "system") {
        return core::MessageRole::System;
    }
    if (role == "user") {
        return core::MessageRole::User;
    }
    if (role == "assistant") {
        return core::MessageRole::Assistant;
    }
    if (role == "tool") {
        return core::MessageRole::Tool;
    }
    throw std::runtime_error("FileChatMessageHistory: unknown role in file: " + role);
}

json message_to_json(const core::Message& message) {
    json calls = json::array();
    for (const auto& call : message.tool_calls) {
        calls.push_back({{"id", call.id}, {"name", call.tool_name}, {"arguments", call.arguments}});
    }
    json images = json::array();
    for (const auto& image : message.images) {
        images.push_back({
            {"source_type", image.source_type == core::ImageSourceType::Base64 ? "base64" : "url"},
            {"data", image.data},
            {"media_type", image.media_type},
        });
    }
    return json{
        {"role", core::to_api_role(message.role)},
        {"content", message.content},
        {"tool_calls", calls},
        {"tool_call_id", message.tool_call_id},
        {"images", images},
    };
}

core::Message message_from_json(const json& entry) {
    core::Message message{parse_role(entry.at("role").get<std::string>()), entry.value("content", "")};
    message.tool_call_id = entry.value("tool_call_id", "");
    for (const auto& call_json : entry.value("tool_calls", json::array())) {
        core::ToolCall call;
        call.id = call_json.at("id").get<std::string>();
        call.tool_name = call_json.at("name").get<std::string>();
        call.arguments = call_json.value("arguments", json::object());
        message.tool_calls.push_back(std::move(call));
    }
    for (const auto& image_json : entry.value("images", json::array())) {
        core::ImageContent image;
        image.source_type =
            image_json.value("source_type", "url") == "base64" ? core::ImageSourceType::Base64 : core::ImageSourceType::Url;
        image.data = image_json.value("data", "");
        image.media_type = image_json.value("media_type", "");
        message.images.push_back(std::move(image));
    }
    return message;
}

} // namespace

FileChatMessageHistory::FileChatMessageHistory(std::string path) : path_(std::move(path)) { load(); }

void FileChatMessageHistory::load() {
    std::ifstream file(path_);
    if (!file) {
        return; // no history yet -- starting fresh is not an error
    }

    json parsed;
    file >> parsed;
    for (const auto& entry : parsed) {
        messages_.push_back(message_from_json(entry));
    }
}

void FileChatMessageHistory::save() const {
    json array = json::array();
    for (const auto& message : messages_) {
        array.push_back(message_to_json(message));
    }

    std::ofstream file(path_);
    if (!file) {
        throw std::runtime_error("FileChatMessageHistory: could not write to " + path_);
    }
    file << array.dump(2);
}

std::vector<core::Message> FileChatMessageHistory::messages() const { return messages_; }

void FileChatMessageHistory::add_message(const core::Message& message) {
    messages_.push_back(message);
    save();
}

void FileChatMessageHistory::clear() {
    messages_.clear();
    save();
}

} // namespace langchain::llm
