#include "langchain/mcp/mcp_tools.hpp"

#include "langchain/tools/function_tool.hpp"

namespace langchain::mcp {

std::vector<std::shared_ptr<tools::Tool>> as_tools(std::shared_ptr<McpClient> client) {
    std::vector<std::shared_ptr<tools::Tool>> tools;
    for (const auto& info : client->list_tools()) {
        tools.push_back(std::make_shared<tools::FunctionTool>(
            info.name, info.description,
            [client, name = info.name](const nlohmann::json& input) { return client->call_tool(name, input); },
            info.input_schema));
    }
    return tools;
}

} // namespace langchain::mcp
