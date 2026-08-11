#pragma once

#include "langchain/tools/tool.hpp"

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace langchain::tools {

// Holds the set of tools available to an agent, keyed by name, and renders
// them into the JSON shape OpenAI/Anthropic-style function-calling APIs
// expect.
class ToolRegistry {
public:
    // Throws std::invalid_argument if a tool with the same name is already registered.
    void add(std::shared_ptr<Tool> tool);

    std::shared_ptr<Tool> get(const std::string& name) const;
    const std::vector<std::shared_ptr<Tool>>& all() const { return tools_; }

    // [{"type": "function", "function": {"name", "description", "parameters"}}, ...]
    nlohmann::json to_openai_tools_json() const;

private:
    std::vector<std::shared_ptr<Tool>> tools_;
    std::unordered_map<std::string, std::shared_ptr<Tool>> by_name_;
};

} // namespace langchain::tools
