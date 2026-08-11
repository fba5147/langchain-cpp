#include "langchain/tools/function_tool.hpp"
#include "langchain/tools/tool_registry.hpp"

#include <gtest/gtest.h>
#include <stdexcept>

using namespace langchain::tools;

TEST(FunctionTool, CallReturnsFunctionResult) {
    FunctionTool add("add", "adds two numbers",
                      [](const nlohmann::json& input) -> nlohmann::json {
                          return input.at("a").get<int>() + input.at("b").get<int>();
                      });

    auto result = add.call({{"a", 2}, {"b", 3}});
    ASSERT_TRUE(result.ok());
    EXPECT_EQ(result.value(), 5);
}

TEST(FunctionTool, ExceptionsBecomeErrorResults) {
    FunctionTool fail("fail", "always throws",
                       [](const nlohmann::json&) -> nlohmann::json { throw std::invalid_argument("boom"); });

    auto result = fail.call({});
    ASSERT_FALSE(result.ok());
    EXPECT_EQ(result.error().message, "boom");
}

TEST(ToolRegistry, AddGetAndLookupMissing) {
    ToolRegistry registry;
    auto tool = std::make_shared<FunctionTool>(
        "echo", "echoes input", [](const nlohmann::json& input) -> nlohmann::json { return input; });

    registry.add(tool);

    EXPECT_EQ(registry.get("echo"), tool);
    EXPECT_EQ(registry.get("missing"), nullptr);
    EXPECT_EQ(registry.all().size(), 1u);
}

TEST(ToolRegistry, DuplicateNameThrows) {
    ToolRegistry registry;
    registry.add(std::make_shared<FunctionTool>(
        "echo", "first", [](const nlohmann::json& input) -> nlohmann::json { return input; }));

    EXPECT_THROW(registry.add(std::make_shared<FunctionTool>(
                     "echo", "second", [](const nlohmann::json& input) -> nlohmann::json { return input; })),
                 std::invalid_argument);
}

TEST(ToolRegistry, RendersOpenAiToolSchema) {
    ToolRegistry registry;
    registry.add(std::make_shared<FunctionTool>(
        "add", "adds two numbers", [](const nlohmann::json&) -> nlohmann::json { return 0; },
        nlohmann::json{{"type", "object"}}));

    auto schema = registry.to_openai_tools_json();
    ASSERT_EQ(schema.size(), 1u);
    EXPECT_EQ(schema[0]["type"], "function");
    EXPECT_EQ(schema[0]["function"]["name"], "add");
    EXPECT_EQ(schema[0]["function"]["description"], "adds two numbers");
    EXPECT_EQ(schema[0]["function"]["parameters"]["type"], "object");
}

TEST(ToolRegistry, RendersAnthropicToolSchema) {
    ToolRegistry registry;
    registry.add(std::make_shared<FunctionTool>(
        "add", "adds two numbers", [](const nlohmann::json&) -> nlohmann::json { return 0; },
        nlohmann::json{{"type", "object"}}));

    auto schema = registry.to_anthropic_tools_json();
    ASSERT_EQ(schema.size(), 1u);
    EXPECT_EQ(schema[0]["name"], "add");
    EXPECT_EQ(schema[0]["description"], "adds two numbers");
    EXPECT_EQ(schema[0]["input_schema"]["type"], "object");
}

TEST(ToolRegistry, RendersGeminiToolSchema) {
    ToolRegistry registry;
    registry.add(std::make_shared<FunctionTool>(
        "add", "adds two numbers", [](const nlohmann::json&) -> nlohmann::json { return 0; },
        nlohmann::json{{"type", "object"}}));

    auto schema = registry.to_gemini_tools_json();
    ASSERT_EQ(schema.size(), 1u);
    auto declarations = schema[0]["functionDeclarations"];
    ASSERT_EQ(declarations.size(), 1u);
    EXPECT_EQ(declarations[0]["name"], "add");
    EXPECT_EQ(declarations[0]["description"], "adds two numbers");
    EXPECT_EQ(declarations[0]["parameters"]["type"], "object");
}
