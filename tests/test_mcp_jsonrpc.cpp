// Direct unit tests for the generic JSON-RPC 2.0 framing and the
// MCP-specific message shapes layered on top of it -- both pure
// functions, testable without spawning a real MCP server (see
// test_mcp_stdio_transport.cpp for the transport itself, and
// examples/mcp_client_demo.cpp for a full live run).

#include "mcp/jsonrpc.hpp"
#include "mcp/mcp_protocol.hpp"

#include "langchain/tools/function_tool.hpp"

#include <gtest/gtest.h>

using namespace langchain::mcp::detail;
using json = nlohmann::json;

TEST(BuildRequest, ProducesJsonRpc2Shape) {
    auto request = build_request(1, "tools/list", json::object());
    EXPECT_EQ(request["jsonrpc"], "2.0");
    EXPECT_EQ(request["id"], 1);
    EXPECT_EQ(request["method"], "tools/list");
    EXPECT_TRUE(request["params"].is_object());
}

TEST(BuildNotification, HasNoId) {
    auto notification = build_notification("notifications/initialized", json::object());
    EXPECT_FALSE(notification.contains("id"));
    EXPECT_EQ(notification["method"], "notifications/initialized");
}

TEST(IsResponse, TrueForMessageWithIdAndNoMethod) {
    EXPECT_TRUE(is_response(json{{"id", 1}, {"result", {{"a", 1}}}}));
    EXPECT_TRUE(is_response(json{{"id", 1}, {"error", {{"code", -1}, {"message", "boom"}}}}));
}

TEST(IsResponse, FalseForRequestOrNotificationFromServer) {
    EXPECT_FALSE(is_response(json{{"id", 1}, {"method", "sampling/createMessage"}, {"params", {}}}));
    EXPECT_FALSE(is_response(json{{"method", "notifications/message"}, {"params", {}}}));
}

TEST(ParseResponse, SuccessResultIsExtracted) {
    auto response = parse_response(json{{"id", 1}, {"result", {{"tools", json::array()}}}});
    EXPECT_FALSE(response.is_error);
    EXPECT_TRUE(response.result.contains("tools"));
}

TEST(ParseResponse, ErrorIsFlaggedWithMessage) {
    auto response = parse_response(json{{"id", 1}, {"error", {{"code", -32601}, {"message", "method not found"}}}});
    EXPECT_TRUE(response.is_error);
    EXPECT_EQ(response.error_message, "method not found");
}

TEST(BuildInitializeParams, IncludesProtocolVersionAndClientInfo) {
    auto params = build_initialize_params();
    EXPECT_TRUE(params.contains("protocolVersion"));
    EXPECT_EQ(params["clientInfo"]["name"], "langchain-cpp");
}

TEST(ParseToolsListResult, ExtractsNameDescriptionAndSchema) {
    json result{{"tools",
                 {{{"name", "calculator"},
                   {"description", "Evaluates an expression"},
                   {"inputSchema", {{"type", "object"}}}}}}};

    auto tools = parse_tools_list_result(result);

    ASSERT_EQ(tools.size(), 1u);
    EXPECT_EQ(tools[0].name, "calculator");
    EXPECT_EQ(tools[0].description, "Evaluates an expression");
    EXPECT_EQ(tools[0].input_schema["type"], "object");
}

TEST(ParseToolsListResult, DescriptionAndSchemaDefaultWhenMissing) {
    json result{{"tools", {{{"name", "no_frills"}}}}};

    auto tools = parse_tools_list_result(result);

    ASSERT_EQ(tools.size(), 1u);
    EXPECT_EQ(tools[0].description, "");
    EXPECT_EQ(tools[0].input_schema["type"], "object");
}

TEST(ParseCallToolResult, SingleTextBlockBecomesJsonString) {
    json result{{"content", {{{"type", "text"}, {"text", "56088"}}}}};
    auto value = parse_call_tool_result(result);
    ASSERT_TRUE(value.is_string());
    EXPECT_EQ(value.get<std::string>(), "56088");
}

TEST(ParseCallToolResult, MultipleBlocksReturnRawContentArray) {
    json result{{"content", {{{"type", "text"}, {"text", "a"}}, {{"type", "text"}, {"text", "b"}}}}};
    auto value = parse_call_tool_result(result);
    ASSERT_TRUE(value.is_array());
    EXPECT_EQ(value.size(), 2u);
}

TEST(ParseCallToolResult, ThrowsWithToolsOwnErrorTextWhenIsErrorTrue) {
    json result{{"isError", true}, {"content", {{{"type", "text"}, {"text", "division by zero"}}}}};
    EXPECT_THROW(
        {
            try {
                parse_call_tool_result(result);
            } catch (const std::runtime_error& error) {
                EXPECT_STREQ(error.what(), "division by zero");
                throw;
            }
        },
        std::runtime_error);
}

TEST(ParseCallToolResult, ThrowsGenericMessageWhenIsErrorTrueWithNoTextBlock) {
    json result{{"isError", true}, {"content", json::array()}};
    EXPECT_THROW(parse_call_tool_result(result), std::runtime_error);
}

TEST(IsRequest, TrueForMessageWithMethodAndId) {
    EXPECT_TRUE(is_request(json{{"id", 1}, {"method", "tools/list"}, {"params", json::object()}}));
}

TEST(IsRequest, FalseForNotificationOrResponse) {
    EXPECT_FALSE(is_request(json{{"method", "notifications/initialized"}, {"params", json::object()}}));
    EXPECT_FALSE(is_request(json{{"id", 1}, {"result", json::object()}}));
}

TEST(ParseRequest, ExtractsIdMethodAndParams) {
    auto request = parse_request(json{{"id", 7}, {"method", "tools/call"}, {"params", {{"name", "calculator"}}}});
    EXPECT_EQ(request.id, 7);
    EXPECT_EQ(request.method, "tools/call");
    EXPECT_EQ(request.params["name"], "calculator");
}

TEST(BuildSuccessResponse, EchoesIdAndCarriesResult) {
    auto response = build_success_response(7, json{{"tools", json::array()}});
    EXPECT_EQ(response["id"], 7);
    EXPECT_TRUE(response["result"].contains("tools"));
    EXPECT_FALSE(response.contains("error"));
}

TEST(BuildErrorResponse, EchoesIdAndCarriesCodeAndMessage) {
    auto response = build_error_response(7, -32602, "Tool add not found");
    EXPECT_EQ(response["id"], 7);
    EXPECT_EQ(response["error"]["code"], -32602);
    EXPECT_EQ(response["error"]["message"], "Tool add not found");
}

TEST(BuildInitializeResult, IncludesProtocolVersionCapabilitiesAndServerInfo) {
    auto result = build_initialize_result();
    EXPECT_TRUE(result.contains("protocolVersion"));
    EXPECT_TRUE(result["capabilities"].contains("tools"));
    EXPECT_EQ(result["serverInfo"]["name"], "langchain-cpp");
}

TEST(BuildToolsListResult, ListsEachToolsNameDescriptionAndSchema) {
    std::vector<std::shared_ptr<langchain::tools::Tool>> tools{std::make_shared<langchain::tools::FunctionTool>(
        "calculator", "Evaluates an expression", [](const json&) -> json { return 0; },
        json{{"type", "object"}})};

    auto result = build_tools_list_result(tools);

    ASSERT_EQ(result["tools"].size(), 1u);
    EXPECT_EQ(result["tools"][0]["name"], "calculator");
    EXPECT_EQ(result["tools"][0]["description"], "Evaluates an expression");
    EXPECT_EQ(result["tools"][0]["inputSchema"]["type"], "object");
}

TEST(BuildCallToolResult, StringValueBecomesTextVerbatim) {
    auto result = build_call_tool_result(langchain::core::Result<json>(json("sunny")));
    EXPECT_FALSE(result.value("isError", false));
    EXPECT_EQ(result["content"][0]["text"], "sunny");
}

TEST(BuildCallToolResult, NonStringValueIsJsonDumpedIntoText) {
    auto result = build_call_tool_result(langchain::core::Result<json>(json(42)));
    EXPECT_EQ(result["content"][0]["text"], "42");
}

TEST(BuildCallToolResult, ErrorBecomesIsErrorTrueWithMessageAsText) {
    auto result = build_call_tool_result(langchain::core::Result<json>(langchain::core::Error{"division by zero"}));
    EXPECT_TRUE(result.value("isError", false));
    EXPECT_EQ(result["content"][0]["text"], "division by zero");
}
