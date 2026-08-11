#include "langchain/agents/agent_executor.hpp"
#include "langchain/providers/mock/mock_chat.hpp"
#include "langchain/tools/function_tool.hpp"
#include "langchain/tools/tool_registry.hpp"

#include <gtest/gtest.h>
#include <stdexcept>

using namespace langchain::agents;
using namespace langchain::core;
using namespace langchain::providers;
using namespace langchain::tools;

namespace {

std::shared_ptr<ToolRegistry> make_add_tool_registry() {
    auto registry = std::make_shared<ToolRegistry>();
    registry->add(std::make_shared<FunctionTool>("add", "adds two numbers", [](const nlohmann::json& input) -> nlohmann::json {
        return input.at("a").get<int>() + input.at("b").get<int>();
    }));
    return registry;
}

} // namespace

TEST(AgentExecutor, RunsToolThenReturnsFinalAnswer) {
    auto model = std::make_shared<MockChat>(std::vector<Message>{
        Message::assistant_tool_calls({ToolCall{"call_1", "add", {{"a", 2}, {"b", 3}}}}),
        Message::assistant("The answer is 5."),
    });

    AgentExecutor agent(model, make_add_tool_registry());
    Message result = agent.run("What is 2 + 3?");

    EXPECT_EQ(result.role, MessageRole::Assistant);
    EXPECT_EQ(result.content, "The answer is 5.");
    EXPECT_FALSE(result.has_tool_calls());
}

TEST(AgentExecutor, UnknownToolReportsErrorButLoopContinues) {
    auto model = std::make_shared<MockChat>(std::vector<Message>{
        Message::assistant_tool_calls({ToolCall{"call_1", "missing_tool", {}}}),
        Message::assistant("Sorry, I couldn't find that tool."),
    });

    AgentExecutor agent(model, make_add_tool_registry());
    Message result = agent.run("Do something unsupported.");

    EXPECT_EQ(result.content, "Sorry, I couldn't find that tool.");
}

TEST(AgentExecutor, ThrowsWhenMaxStepsExceeded) {
    // A single scripted tool-call response that MockChat will keep
    // replaying forever, so the agent never gets a final answer.
    auto model = std::make_shared<MockChat>(
        std::vector<Message>{Message::assistant_tool_calls({ToolCall{"call_1", "add", {{"a", 1}, {"b", 1}}}})});

    AgentExecutor agent(model, make_add_tool_registry(), AgentConfig{2});
    EXPECT_THROW(agent.run("loop forever"), std::runtime_error);
}
