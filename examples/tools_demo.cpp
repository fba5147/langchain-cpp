// Demonstrates defining a Tool an agent could later call, registering it
// in a ToolRegistry, and rendering the OpenAI-style function-calling
// schema an agent loop would send to the model.

#include "langchain/tools/function_tool.hpp"
#include "langchain/tools/tool_registry.hpp"

#include <iostream>
#include <stdexcept>

using namespace langchain;

int main() {
    auto calculator = std::make_shared<tools::FunctionTool>(
        "calculator", "Evaluates a simple `a <op> b` arithmetic expression.",
        [](const nlohmann::json& input) -> nlohmann::json {
            double a = input.at("a").get<double>();
            double b = input.at("b").get<double>();
            std::string op = input.at("op").get<std::string>();
            if (op == "+") return a + b;
            if (op == "-") return a - b;
            if (op == "*") return a * b;
            if (op == "/") {
                if (b == 0) {
                    throw std::invalid_argument("division by zero");
                }
                return a / b;
            }
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
        });

    tools::ToolRegistry registry;
    registry.add(calculator);

    auto result = registry.get("calculator")->call({{"a", 123}, {"b", 456}, {"op", "*"}});
    if (result) {
        std::cout << "123 * 456 = " << result.value() << '\n';
    }

    auto bad_result = registry.get("calculator")->call({{"a", 1}, {"b", 0}, {"op", "/"}});
    if (!bad_result) {
        std::cout << "1 / 0 -> error: " << bad_result.error().message << '\n';
    }

    std::cout << "\nOpenAI-style tool schema:\n" << registry.to_openai_tools_json().dump(2) << '\n';

    return 0;
}
