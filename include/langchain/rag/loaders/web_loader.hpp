#pragma once

#include "langchain/rag/loaders/loader.hpp"

#include <string>

namespace langchain::rag {

// Fetches a URL and loads it as a single Document, with HTML tags
// stripped to leave plain, readable text (script/style content is
// dropped entirely, not just the surrounding tags). This is a minimal
// tag-stripping conversion, not a real HTML parser -- good enough for a
// typical article/doc page, not guaranteed on heavily scripted or
// malformed markup. metadata["source"] is set to the URL. Throws
// std::runtime_error on a non-200 response or a request failure.
class WebLoader : public DocumentLoader {
public:
    explicit WebLoader(std::string url);

    std::vector<core::Document> load() override;

private:
    std::string url_;
};

} // namespace langchain::rag
