#include "langchain/tools/tool_registry.hpp"

#include <stdexcept>

namespace langchain::tools {

void ToolRegistry::add(std::shared_ptr<Tool> tool) {
    const std::string name = tool->name();
    if (by_name_.find(name) != by_name_.end()) {
        throw std::invalid_argument("ToolRegistry: a tool named '" + name + "' is already registered");
    }
    by_name_.emplace(name, tool);
    tools_.push_back(std::move(tool));
}

std::shared_ptr<Tool> ToolRegistry::get(const std::string& name) const {
    auto it = by_name_.find(name);
    return it == by_name_.end() ? nullptr : it->second;
}

nlohmann::json ToolRegistry::to_openai_tools_json() const {
    nlohmann::json tools = nlohmann::json::array();
    for (const auto& tool : tools_) {
        tools.push_back({
            {"type", "function"},
            {"function",
             {
                 {"name", tool->name()},
                 {"description", tool->description()},
                 {"parameters", tool->parameters_schema()},
             }},
        });
    }
    return tools;
}

nlohmann::json ToolRegistry::to_anthropic_tools_json() const {
    nlohmann::json tools = nlohmann::json::array();
    for (const auto& tool : tools_) {
        tools.push_back({
            {"name", tool->name()},
            {"description", tool->description()},
            {"input_schema", tool->parameters_schema()},
        });
    }
    return tools;
}

} // namespace langchain::tools
