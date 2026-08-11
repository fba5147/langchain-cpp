#include "langchain/mcp/mcp_server.hpp"

#include "jsonrpc.hpp"
#include "mcp_protocol.hpp"

namespace langchain::mcp {

using json = nlohmann::json;

McpServer::McpServer(std::shared_ptr<tools::ToolRegistry> registry) : registry_(std::move(registry)) {}

void McpServer::serve(std::istream& in, std::ostream& out) {
    std::string line;
    while (std::getline(in, line)) {
        if (line.empty()) {
            continue;
        }

        json message;
        try {
            message = json::parse(line);
        } catch (const json::parse_error&) {
            continue; // malformed input -- ignore rather than crash the server
        }

        if (!detail::is_request(message)) {
            continue; // a notification (e.g. notifications/initialized) needs no response
        }

        detail::JsonRpcRequest request = detail::parse_request(message);
        json response;
        try {
            if (request.method == "initialize") {
                response = detail::build_success_response(request.id, detail::build_initialize_result());
            } else if (request.method == "tools/list") {
                response =
                    detail::build_success_response(request.id, detail::build_tools_list_result(registry_->all()));
            } else if (request.method == "tools/call") {
                std::string name = request.params.at("name").get<std::string>();
                json arguments = request.params.value("arguments", json::object());
                auto tool = registry_->get(name);
                if (!tool) {
                    response = detail::build_error_response(request.id, -32602, "Tool " + name + " not found");
                } else {
                    response =
                        detail::build_success_response(request.id, detail::build_call_tool_result(tool->call(arguments)));
                }
            } else {
                response = detail::build_error_response(request.id, -32601, "Method not found: " + request.method);
            }
        } catch (const std::exception& error) {
            response = detail::build_error_response(request.id, -32603, error.what());
        }

        out << response.dump() << "\n";
        out.flush();
    }
}

} // namespace langchain::mcp
