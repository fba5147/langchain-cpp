#include "mcp_protocol.hpp"

#include <stdexcept>

namespace langchain::mcp::detail {

using json = nlohmann::json;

json build_initialize_params() {
    return json{
        {"protocolVersion", "2024-11-05"},
        {"capabilities", json::object()},
        {"clientInfo", {{"name", "langchain-cpp"}, {"version", "0.17.0"}}},
    };
}

std::vector<McpToolInfo> parse_tools_list_result(const json& result) {
    std::vector<McpToolInfo> tools;
    for (const auto& tool_json : result.value("tools", json::array())) {
        McpToolInfo tool;
        tool.name = tool_json.at("name").get<std::string>();
        tool.description = tool_json.value("description", "");
        tool.input_schema = tool_json.value("inputSchema", json{{"type", "object"}});
        tools.push_back(std::move(tool));
    }
    return tools;
}

json parse_call_tool_result(const json& result) {
    json content = result.value("content", json::array());

    if (result.value("isError", false)) {
        std::string message = "MCP tool call failed";
        if (!content.empty() && content[0].value("type", "") == "text") {
            message = content[0].value("text", message);
        }
        throw std::runtime_error(message);
    }

    if (content.size() == 1 && content[0].value("type", "") == "text") {
        return json(content[0].value("text", ""));
    }
    return content;
}

json build_initialize_result() {
    return json{
        {"protocolVersion", "2024-11-05"},
        {"capabilities", {{"tools", json::object()}}},
        {"serverInfo", {{"name", "langchain-cpp"}, {"version", "0.17.0"}}},
    };
}

json build_tools_list_result(const std::vector<std::shared_ptr<tools::Tool>>& tools) {
    json tools_json = json::array();
    for (const auto& tool : tools) {
        tools_json.push_back({
            {"name", tool->name()},
            {"description", tool->description()},
            {"inputSchema", tool->parameters_schema()},
        });
    }
    return json{{"tools", tools_json}};
}

json build_call_tool_result(const core::Result<json>& result) {
    if (!result.ok()) {
        return json{
            {"isError", true},
            {"content", json::array({json{{"type", "text"}, {"text", result.error().message}}})},
        };
    }

    const json& value = result.value();
    std::string text = value.is_string() ? value.get<std::string>() : value.dump();
    return json{{"content", json::array({json{{"type", "text"}, {"text", text}}})}};
}

} // namespace langchain::mcp::detail
