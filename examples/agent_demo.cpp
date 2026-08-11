// Demonstrates AgentExecutor's LLM <-> tool loop with a calculator tool.
// Uses a scripted MockChat by default (deterministic, no network); set
// ANTHROPIC_API_KEY or OPENAI_API_KEY to see a live model actually decide
// to call the tool.

#include "langchain/langchain.hpp"

#include <cstdlib>
#include <iostream>
#include <stdexcept>

using namespace langchain;

namespace {

std::shared_ptr<tools::ToolRegistry> make_tools() {
    auto registry = std::make_shared<tools::ToolRegistry>();
    registry->add(std::make_shared<tools::FunctionTool>(
        "calculator", "Evaluates a simple `a <op> b` arithmetic expression.",
        [](const nlohmann::json& input) -> nlohmann::json {
            double a = input.at("a").get<double>();
            double b = input.at("b").get<double>();
            std::string op = input.at("op").get<std::string>();
            if (op == "+") return a + b;
            if (op == "-") return a - b;
            if (op == "*") return a * b;
            if (op == "/") return a / b;
            throw std::invalid_argument("unknown op: " + op);
        },
        nlohmann::json{
            {"type", "object"},
            {"properties",
             {
                 {"a", {{"type", "number"}}},
                 {"b", {{"type", "number"}}},
                 {"op", {{"type", "string"}, {"enum", {"+", "-", "*", "/"}}}},
             }},
            {"required", {"a", "b", "op"}},
        }));
    return registry;
}

// A model that, when asked "what is 123 * 456?", requests the calculator
// tool once and then answers using its result.
std::shared_ptr<llm::ChatModel> make_scripted_mock() {
    return std::make_shared<providers::MockChat>(std::vector<core::Message>{
        core::Message::assistant_tool_calls(
            {core::ToolCall{"call_1", "calculator", {{"a", 123}, {"b", 456}, {"op", "*"}}}}),
        core::Message::assistant("123 * 456 is 56088."),
    });
}

} // namespace

int main() {
    auto tool_registry = make_tools();

    std::shared_ptr<llm::ChatModel> model;
    if (std::getenv("ANTHROPIC_API_KEY") != nullptr) {
        model = std::make_shared<providers::AnthropicChat>();
    } else if (std::getenv("OPENAI_API_KEY") != nullptr) {
        model = std::make_shared<providers::OpenAIChat>();
    } else {
        std::cout << "No ANTHROPIC_API_KEY or OPENAI_API_KEY set; using a scripted MockChat instead.\n\n";
        model = make_scripted_mock();
    }

    agents::AgentExecutor agent(model, tool_registry);
    core::Message answer = agent.run("What is 123 * 456?");

    std::cout << answer.content << '\n';
    return 0;
}
