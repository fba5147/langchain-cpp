#include "openai_wire_format.hpp"

namespace langchain::providers::detail {

using json = nlohmann::json;

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

} // namespace langchain::providers::detail
