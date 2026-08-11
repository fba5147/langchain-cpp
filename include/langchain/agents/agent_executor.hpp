#pragma once

#include "langchain/core/message.hpp"
#include "langchain/llm/chat_model.hpp"
#include "langchain/tools/tool_registry.hpp"

#include <memory>
#include <string>
#include <vector>

namespace langchain::agents {

struct AgentConfig {
    // Upper bound on model round-trips before giving up, so a model that
    // keeps requesting tools can't loop forever.
    int max_steps = 10;
};

// Runs the LLM <-> tool loop: send messages to a tool-bound model; if it
// asks for tool calls, execute each one and feed the results back as
// Message::tool_result entries; repeat until it returns a plain answer or
// max_steps is exceeded.
class AgentExecutor {
public:
    AgentExecutor(std::shared_ptr<llm::ChatModel> model, std::shared_ptr<tools::ToolRegistry> tools,
                  AgentConfig config = {});

    // Runs the loop starting from `messages` (typically a system prompt
    // plus one user message) and returns the final assistant message.
    // Throws std::runtime_error if max_steps is exceeded without a final
    // answer.
    core::Message run(std::vector<core::Message> messages);

    // Convenience: wraps a single user message in a fresh conversation.
    core::Message run(const std::string& user_input);

private:
    std::shared_ptr<llm::ChatModel> bound_model_;
    std::shared_ptr<tools::ToolRegistry> tools_;
    AgentConfig config_;
};

} // namespace langchain::agents
