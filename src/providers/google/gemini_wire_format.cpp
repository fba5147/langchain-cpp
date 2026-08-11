#include "gemini_wire_format.hpp"

#include <stdexcept>
#include <unordered_map>

namespace langchain::providers::detail {

using json = nlohmann::json;

GeminiRequest messages_to_gemini_request(const std::vector<core::Message>& messages) {
    GeminiRequest request;
    request.contents = json::array();

    // Populated as we walk assistant tool-call messages, so that when we
    // reach the Tool-role message answering one of those calls, we can
    // recover the function name Gemini's functionResponse needs (Gemini
    // correlates by name, not by an id the way OpenAI/Anthropic do).
    std::unordered_map<std::string, std::string> call_id_to_name;

    for (const auto& message : messages) {
        if (message.role == core::MessageRole::System) {
            if (!request.system_instruction.empty()) {
                request.system_instruction += "\n";
            }
            request.system_instruction += message.content;
            continue;
        }

        if (message.role == core::MessageRole::Tool) {
            auto it = call_id_to_name.find(message.tool_call_id);
            std::string name = it != call_id_to_name.end() ? it->second : message.tool_call_id;

            json result_value;
            try {
                result_value = json::parse(message.content);
            } catch (const json::parse_error&) {
                result_value = message.content;
            }

            request.contents.push_back({
                {"role", "function"},
                {"parts", json::array({json{
                              {"functionResponse", {{"name", name}, {"response", {{"result", result_value}}}}},
                          }})},
            });
            continue;
        }

        if (!message.tool_calls.empty()) {
            json parts = json::array();
            if (!message.content.empty()) {
                parts.push_back({{"text", message.content}});
            }
            for (const auto& call : message.tool_calls) {
                call_id_to_name[call.id] = call.tool_name;
                parts.push_back({{"functionCall", {{"name", call.tool_name}, {"args", call.arguments}}}});
            }
            request.contents.push_back({{"role", "model"}, {"parts", parts}});
            continue;
        }

        std::string role = message.role == core::MessageRole::Assistant ? "model" : "user";
        request.contents.push_back({{"role", role}, {"parts", json::array({json{{"text", message.content}}})}});
    }

    return request;
}

core::Message parse_gemini_message(const json& response_json) {
    if (!response_json.contains("candidates") || response_json["candidates"].empty()) {
        throw std::runtime_error("GeminiChat: response has no candidates: " + response_json.dump());
    }

    const json& parts = response_json["candidates"][0]["content"]["parts"];

    std::string text;
    std::vector<core::ToolCall> calls;
    int next_id = 0;

    for (const auto& part : parts) {
        if (part.contains("text")) {
            text += part["text"].get<std::string>();
        } else if (part.contains("functionCall")) {
            core::ToolCall call;
            // Synthesize an id -- Gemini's protocol has none -- so the
            // rest of the library's id-based tool_result correlation
            // still works uniformly across providers.
            call.id = "call_" + std::to_string(next_id++);
            call.tool_name = part["functionCall"]["name"].get<std::string>();
            call.arguments = part["functionCall"].value("args", json::object());
            calls.push_back(std::move(call));
        }
    }

    if (!calls.empty()) {
        return core::Message::assistant_tool_calls(std::move(calls), text);
    }
    return core::Message::assistant(text);
}

} // namespace langchain::providers::detail
