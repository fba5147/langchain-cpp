#include "langchain/agents/agent_executor.hpp"

#include <nlohmann/json.hpp>

#include <stdexcept>

namespace langchain::agents {

AgentExecutor::AgentExecutor(std::shared_ptr<llm::ChatModel> model, std::shared_ptr<tools::ToolRegistry> tools,
                              AgentConfig config)
    : bound_model_(model->bind_tools(tools)), tools_(std::move(tools)), config_(config) {}

core::Message AgentExecutor::run(std::vector<core::Message> messages) {
    for (int step = 0; step < config_.max_steps; ++step) {
        core::Message reply = bound_model_->invoke(messages);

        if (!reply.has_tool_calls()) {
            return reply;
        }

        messages.push_back(reply);

        for (const auto& call : reply.tool_calls) {
            auto tool = tools_->get(call.tool_name);

            std::string result_content;
            if (!tool) {
                result_content = nlohmann::json{{"error", "unknown tool: " + call.tool_name}}.dump();
            } else {
                core::Result<nlohmann::json> result = tool->call(call.arguments);
                result_content = result.ok() ? result.value().dump()
                                              : nlohmann::json{{"error", result.error().message}}.dump();
            }

            messages.push_back(core::Message::tool_result(call.id, result_content));
        }
    }

    throw std::runtime_error("AgentExecutor: exceeded max_steps (" + std::to_string(config_.max_steps) +
                              ") without a final answer");
}

core::Message AgentExecutor::run(const std::string& user_input) {
    return run(std::vector<core::Message>{core::Message::user(user_input)});
}

} // namespace langchain::agents
