#include "langchain/mcp/mcp_client.hpp"

#include "client_transport.hpp"
#include "http_client_transport.hpp"
#include "jsonrpc.hpp"
#include "mcp_protocol.hpp"
#include "stdio_client_transport.hpp"

namespace langchain::mcp {

using json = nlohmann::json;

McpClient::McpClient(std::vector<std::string> command)
    : transport_(std::make_unique<detail::StdioClientTransport>(std::move(command))) {}

McpClient::McpClient(McpHttpConfig config)
    : transport_(std::make_unique<detail::HttpClientTransport>(
          detail::HttpClientTransportConfig{std::move(config.url)})) {}

McpClient::~McpClient() = default;

json McpClient::send_request(const std::string& method, const json& params) {
    return transport_->send_request(detail::build_request(next_id_++, method, params));
}

void McpClient::send_notification(const std::string& method, const json& params) {
    transport_->send_notification(detail::build_notification(method, params));
}

void McpClient::initialize() {
    send_request("initialize", detail::build_initialize_params());
    send_notification("notifications/initialized", json::object());
}

std::vector<McpToolInfo> McpClient::list_tools() {
    return detail::parse_tools_list_result(send_request("tools/list", json::object()));
}

json McpClient::call_tool(const std::string& name, const json& arguments) {
    json result = send_request("tools/call", json{{"name", name}, {"arguments", arguments}});
    return detail::parse_call_tool_result(result);
}

} // namespace langchain::mcp
