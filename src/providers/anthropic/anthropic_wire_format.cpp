#include "anthropic_wire_format.hpp"

#include <stdexcept>

namespace langchain::providers::detail {

using json = nlohmann::json;

json message_to_anthropic_json(const core::Message& message) {
    if (!message.images.empty()) {
        throw std::runtime_error(
            "AnthropicChat: image content is not yet supported by this provider implementation");
    }

    if (message.role == core::MessageRole::Tool) {
        return json{
            {"role", "user"},
            {"content", json::array({json{
                            {"type", "tool_result"},
                            {"tool_use_id", message.tool_call_id},
                            {"content", message.content},
                        }})},
        };
    }

    if (!message.tool_calls.empty()) {
        json blocks = json::array();
        if (!message.content.empty()) {
            blocks.push_back({{"type", "text"}, {"text", message.content}});
        }
        for (const auto& call : message.tool_calls) {
            blocks.push_back({
                {"type", "tool_use"},
                {"id", call.id},
                {"name", call.tool_name},
                {"input", call.arguments},
            });
        }
        return json{{"role", core::to_api_role(message.role)}, {"content", blocks}};
    }

    return json{{"role", core::to_api_role(message.role)}, {"content", message.content}};
}

core::Message parse_anthropic_message(const json& parsed) {
    std::string text;
    std::vector<core::ToolCall> calls;

    for (const auto& block : parsed.at("content")) {
        std::string type = block.value("type", "");
        if (type == "text") {
            text += block.value("text", "");
        } else if (type == "tool_use") {
            core::ToolCall call;
            call.id = block.at("id").get<std::string>();
            call.tool_name = block.at("name").get<std::string>();
            call.arguments = block.value("input", json::object());
            calls.push_back(std::move(call));
        }
    }

    if (!calls.empty()) {
        return core::Message::assistant_tool_calls(std::move(calls), text);
    }
    return core::Message::assistant(text);
}

} // namespace langchain::providers::detail
