#pragma once

#include <string>

// Internal implementation detail: lives under src/, not include/, and is
// not part of the public API. Split out purely so the URL construction
// (the one part of the Azure integration testable without a real Azure
// resource -- the request/response body shape is shared with, and
// already verified via, OpenAIChat) has a unit test.
namespace langchain::providers::detail {

inline std::string build_azure_chat_completions_url(const std::string& endpoint, const std::string& deployment,
                                                      const std::string& api_version) {
    return endpoint + "/openai/deployments/" + deployment + "/chat/completions?api-version=" + api_version;
}

} // namespace langchain::providers::detail
