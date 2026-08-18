#include "stdio_client_transport.hpp"

#include "jsonrpc.hpp"

#include <stdexcept>

namespace langchain::mcp::detail {

using json = nlohmann::json;

StdioClientTransport::StdioClientTransport(std::vector<std::string> command)
    : transport_(std::make_unique<StdioTransport>(std::move(command))) {}

json StdioClientTransport::send_request(const json& request) {
    int id = request.at("id").get<int>();
    transport_->write_line(request.dump());

    while (true) {
        auto line = transport_->read_line();
        if (!line.has_value()) {
            throw std::runtime_error("StdioClientTransport: server closed its output before responding to " +
                                      request.value("method", std::string("request")));
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
        if (!is_response(message) || message.value("id", -1) != id) {
            continue;
        }

        JsonRpcResponse response = parse_response(message);
        if (response.is_error) {
            throw std::runtime_error("StdioClientTransport: request failed: " + response.error_message);
        }
        return response.result;
    }
}

void StdioClientTransport::send_notification(const json& notification) { transport_->write_line(notification.dump()); }

} // namespace langchain::mcp::detail
