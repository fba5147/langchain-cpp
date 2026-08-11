#include "jsonrpc.hpp"

namespace langchain::mcp::detail {

using json = nlohmann::json;

json build_request(int id, const std::string& method, const json& params) {
    return json{{"jsonrpc", "2.0"}, {"id", id}, {"method", method}, {"params", params}};
}

json build_notification(const std::string& method, const json& params) {
    return json{{"jsonrpc", "2.0"}, {"method", method}, {"params", params}};
}

bool is_response(const json& message) {
    return message.contains("id") && !message.contains("method");
}

JsonRpcResponse parse_response(const json& message) {
    JsonRpcResponse response;
    if (message.contains("error")) {
        response.is_error = true;
        response.error_message = message["error"].value("message", "unknown error");
    } else {
        response.result = message.value("result", json::object());
    }
    return response;
}

bool is_request(const json& message) { return message.contains("method") && message.contains("id"); }

JsonRpcRequest parse_request(const json& message) {
    JsonRpcRequest request;
    request.id = message.at("id");
    request.method = message.at("method").get<std::string>();
    request.params = message.value("params", json::object());
    return request;
}

json build_success_response(const json& id, const json& result) {
    return json{{"jsonrpc", "2.0"}, {"id", id}, {"result", result}};
}

json build_error_response(const json& id, int code, const std::string& message) {
    return json{{"jsonrpc", "2.0"}, {"id", id}, {"error", {{"code", code}, {"message", message}}}};
}

} // namespace langchain::mcp::detail
