#pragma once

#include "langchain/mcp/mcp_client.hpp"
#include "langchain/tools/tool.hpp"

#include <memory>
#include <vector>

namespace langchain::mcp {

// Lists every tool a connected McpClient exposes and wraps each as a
// langchain::tools::Tool (a FunctionTool that calls back into the
// client), so they can be added to a ToolRegistry and called by
// AgentExecutor like any other tool -- mirrors LangChain.js's
// loadMcpTools(). `client` must have had initialize() called already.
std::vector<std::shared_ptr<tools::Tool>> as_tools(std::shared_ptr<McpClient> client);

} // namespace langchain::mcp
