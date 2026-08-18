#pragma once

// ClientTransport implementation over the stdio line transport
// (stdio_transport.hpp). Owns the "wait for the response with a matching
// id, skipping anything else that arrives on the same stream in the
// meantime" logic -- necessary here because stdio is a single shared duplex
// stream where server notifications/requests can interleave with the
// response we're waiting for; HTTP's request/response pairing doesn't have
// this problem, see http_client_transport.hpp.

#include "client_transport.hpp"
#include "stdio_transport.hpp"

#include <memory>
#include <vector>

namespace langchain::mcp::detail {

class StdioClientTransport : public ClientTransport {
public:
    explicit StdioClientTransport(std::vector<std::string> command);

    nlohmann::json send_request(const nlohmann::json& request) override;
    void send_notification(const nlohmann::json& notification) override;

private:
    std::unique_ptr<StdioTransport> transport_;
};

} // namespace langchain::mcp::detail
