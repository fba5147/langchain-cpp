#include "http_client_transport.hpp"

#include "jsonrpc.hpp"

#include <cpr/cpr.h>

#include <stdexcept>

namespace langchain::mcp::detail {

using json = nlohmann::json;

std::optional<std::string> extract_last_sse_event_data(const std::string& body) {
    std::optional<std::string> last;
    std::string current;
    bool have_current = false;

    auto finalize_event = [&]() {
        if (have_current) {
            last = current;
        }
        current.clear();
        have_current = false;
    };

    std::size_t pos = 0;
    while (pos <= body.size()) {
        std::size_t newline = body.find('\n', pos);
        std::string line = (newline == std::string::npos) ? body.substr(pos) : body.substr(pos, newline - pos);
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }

        if (line.empty()) {
            finalize_event();
        } else if (line.rfind("data:", 0) == 0) {
            std::string value = line.substr(5);
            if (!value.empty() && value.front() == ' ') {
                value.erase(0, 1);
            }
            current = have_current ? current + "\n" + value : value;
            have_current = true;
        }
        // Other fields (event:, id:, retry:, or ":"-prefixed comments)
        // carry nothing this client needs.

        if (newline == std::string::npos) {
            break;
        }
        pos = newline + 1;
    }
    finalize_event();
    return last;
}

namespace {

cpr::Header build_headers(const HttpClientTransportConfig& config, const std::string& session_id) {
    cpr::Header headers{
        {"Content-Type", "application/json"},
        {"Accept", "application/json, text/event-stream"},
        {"MCP-Protocol-Version", config.protocol_version},
    };
    if (!session_id.empty()) {
        headers["Mcp-Session-Id"] = session_id;
    }
    return headers;
}

} // namespace

HttpClientTransport::HttpClientTransport(HttpClientTransportConfig config) : config_(std::move(config)) {}

json HttpClientTransport::send_request(const json& request) {
    cpr::Response response =
        cpr::Post(cpr::Url{config_.url}, build_headers(config_, session_id_), cpr::Body{request.dump()});

    if (auto it = response.header.find("Mcp-Session-Id"); it != response.header.end()) {
        session_id_ = it->second;
    }

    if (response.status_code != 200) {
        throw std::runtime_error("HttpClientTransport: request failed (HTTP " +
                                  std::to_string(response.status_code) + "): " + response.text);
    }

    std::string content_type = response.header["Content-Type"];
    json message;
    if (content_type.find("text/event-stream") != std::string::npos) {
        auto data = extract_last_sse_event_data(response.text);
        if (!data.has_value()) {
            throw std::runtime_error("HttpClientTransport: SSE response contained no data event");
        }
        message = json::parse(*data);
    } else {
        message = json::parse(response.text);
    }

    JsonRpcResponse parsed = parse_response(message);
    if (parsed.is_error) {
        throw std::runtime_error("HttpClientTransport: request failed: " + parsed.error_message);
    }
    return parsed.result;
}

void HttpClientTransport::send_notification(const json& notification) {
    cpr::Response response =
        cpr::Post(cpr::Url{config_.url}, build_headers(config_, session_id_), cpr::Body{notification.dump()});

    if (response.status_code != 202) {
        throw std::runtime_error("HttpClientTransport: notification failed (HTTP " +
                                  std::to_string(response.status_code) + "): " + response.text);
    }
}

} // namespace langchain::mcp::detail
