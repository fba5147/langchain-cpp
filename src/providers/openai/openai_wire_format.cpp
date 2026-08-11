#include "openai_wire_format.hpp"

namespace langchain::providers::detail {

using json = nlohmann::json;

namespace {

json image_to_openai_json(const core::ImageContent& image) {
    std::string url = image.source_type == core::ImageSourceType::Base64
                           ? "data:" + image.media_type + ";base64," + image.data
                           : image.data;
    return json{{"type", "image_url"}, {"image_url", {{"url", url}}}};
}

// message_json["content"] is `null` (not absent) whenever the assistant
// returns tool_calls instead of text -- json::value() only substitutes its
// default for a missing key, so it throws on a present-but-null one.
std::string content_or_empty(const json& message_json) {
    if (!message_json.contains("content") || message_json["content"].is_null()) {
        return "";
    }
    return message_json["content"].get<std::string>();
}

} // namespace

json message_to_openai_json(const core::Message& message) {
    if (message.role == core::MessageRole::Tool) {
        return json{
            {"role", "tool"},
            {"tool_call_id", message.tool_call_id},
            {"content", message.content},
        };
    }

    json entry{{"role", core::to_api_role(message.role)}};

    if (!message.images.empty()) {
        json parts = json::array();
        if (!message.content.empty()) {
            parts.push_back({{"type", "text"}, {"text", message.content}});
        }
        for (const auto& image : message.images) {
            parts.push_back(image_to_openai_json(image));
        }
        entry["content"] = parts;
        return entry;
    }

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
        return core::Message::assistant_tool_calls(std::move(calls), content_or_empty(message_json));
    }
    return core::Message::assistant(content_or_empty(message_json));
}

bool OpenAiStreamParser::feed(std::string_view data, const std::function<void(const std::string&)>& on_delta) {
    buffer_.append(data);

    std::size_t pos;
    while ((pos = buffer_.find('\n')) != std::string::npos) {
        std::string line = buffer_.substr(0, pos);
        buffer_.erase(0, pos + 1);
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }
        process_line(line, on_delta);
        if (done_) {
            return false;
        }
    }
    return true;
}

void OpenAiStreamParser::process_line(const std::string& line, const std::function<void(const std::string&)>& on_delta) {
    static const std::string prefix = "data: ";
    if (line.rfind(prefix, 0) != 0) {
        return;
    }
    std::string payload = line.substr(prefix.size());
    if (payload == "[DONE]") {
        done_ = true;
        return;
    }

    json parsed;
    try {
        parsed = json::parse(payload);
    } catch (const json::parse_error&) {
        return;
    }

    if (!parsed.contains("choices") || parsed["choices"].empty()) {
        return;
    }
    const json& delta = parsed["choices"][0]["delta"];

    if (delta.contains("content") && delta["content"].is_string()) {
        std::string piece = delta["content"].get<std::string>();
        if (!piece.empty()) {
            text_ += piece;
            on_delta(piece);
        }
    }

    if (delta.contains("tool_calls")) {
        for (const auto& tc : delta["tool_calls"]) {
            int index = tc.value("index", 0);
            ToolCallAccumulator& accumulator = tool_calls_[index];
            if (tc.contains("id")) {
                accumulator.id = tc["id"].get<std::string>();
            }
            if (tc.contains("function")) {
                const json& function = tc["function"];
                if (function.contains("name")) {
                    accumulator.name = function["name"].get<std::string>();
                }
                if (function.contains("arguments")) {
                    accumulator.arguments += function["arguments"].get<std::string>();
                }
            }
        }
    }
}

core::Message OpenAiStreamParser::finish() const {
    if (!tool_calls_.empty()) {
        std::vector<core::ToolCall> calls;
        for (const auto& [index, accumulator] : tool_calls_) {
            (void)index;
            core::ToolCall call;
            call.id = accumulator.id;
            call.tool_name = accumulator.name;
            try {
                call.arguments = accumulator.arguments.empty() ? json::object() : json::parse(accumulator.arguments);
            } catch (const json::parse_error&) {
                call.arguments = json::object();
            }
            calls.push_back(std::move(call));
        }
        return core::Message::assistant_tool_calls(std::move(calls), text_);
    }
    return core::Message::assistant(text_);
}

} // namespace langchain::providers::detail
