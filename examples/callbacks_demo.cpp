// Demonstrates observability via CallbackingChatModel/CallbackingTool:
// wrap a model and a tool once, and every invoke()/stream()/call() on
// them fires events to every registered CallbackHandler -- here,
// ConsoleCallbackHandler, which just prints them. AgentExecutor itself
// needs no changes at all to become observable this way.
//
// Uses a scripted MockChat, so this runs fully offline.

#include "langchain/langchain.hpp"

#include <iostream>
#include <stdexcept>

using namespace langchain;

int main() {
    auto callback_manager = std::make_shared<core::CallbackManager>();
    callback_manager->add_handler(std::make_shared<callbacks::ConsoleCallbackHandler>());

    auto base_model = std::make_shared<providers::MockChat>(std::vector<core::Message>{
        core::Message::assistant_tool_calls({core::ToolCall{"call_1", "calculator", {{"a", 6}, {"b", 7}, {"op", "*"}}}}),
        core::Message::assistant("6 * 7 is 42."),
    });
    auto model = std::make_shared<callbacks::CallbackingChatModel>(base_model, callback_manager);

    auto base_tool = std::make_shared<tools::FunctionTool>(
        "calculator", "Evaluates a simple `a <op> b` arithmetic expression.",
        [](const nlohmann::json& input) -> nlohmann::json {
            double a = input.at("a").get<double>();
            double b = input.at("b").get<double>();
            std::string op = input.at("op").get<std::string>();
            if (op == "*") {
                return a * b;
            }
            throw std::invalid_argument("unsupported op: " + op);
        },
        nlohmann::json{{"type", "object"}});
    auto tool = std::make_shared<callbacks::CallbackingTool>(base_tool, callback_manager);

    auto registry = std::make_shared<tools::ToolRegistry>();
    registry->add(tool);

    agents::AgentExecutor agent(model, registry);
    core::Message answer = agent.run("What is 6 * 7?");

    std::cout << "\nFinal answer: " << answer.content << '\n';
    return 0;
}
