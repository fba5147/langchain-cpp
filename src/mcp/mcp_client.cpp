#include "langchain/mcp/mcp_client.hpp"

#include "jsonrpc.hpp"
#include "mcp_protocol.hpp"
#include "stdio_transport.hpp"

#include <stdexcept>

namespace langchain::mcp {

using json = nlohmann::json;

McpClient::McpClient(std::vector<std::string> command)
    : transport_(std::make_unique<detail::StdioTransport>(std::move(command))) {}

McpClient::~McpClient() = default;

json McpClient::send_request(const std::string& method, const json& params) {
    int id = next_id_++;
    transport_->write_line(detail::build_request(id, method, params).dump());

    while (true) {
        auto line = transport_->read_line();
        if (!line.has_value()) {
            throw std::runtime_error("McpClient: server closed its output before responding to " + method);
        }
        if (line->empty()) {
            continue;
        }

        json message;
        try {
            message = json::parse(*line);
        } catch (const json::parse_error&) {
            continue; // not JSON -- e.g. stray stderr-like output on stdout; skip it.
        }

        // Notifications and server-initiated requests can interleave with
        // our response (e.g. logging notifications); only a message with
        // our id (and no "method") is the response we're waiting for.
        if (!detail::is_response(message) || message.value("id", -1) != id) {
            continue;
        }

        detail::JsonRpcResponse response = detail::parse_response(message);
        if (response.is_error) {
            throw std::runtime_error("McpClient: " + method + " failed: " + response.error_message);
        }
        return response.result;
    }
}

void McpClient::send_notification(const std::string& method, const json& params) {
    transport_->write_line(detail::build_notification(method, params).dump());
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
