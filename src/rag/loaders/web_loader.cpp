#include "langchain/rag/loaders/web_loader.hpp"

#include "html_to_text.hpp"

#include <cpr/cpr.h>

#include <stdexcept>

namespace langchain::rag {

WebLoader::WebLoader(std::string url) : url_(std::move(url)) {}

std::vector<core::Document> WebLoader::load() {
    cpr::Response response = cpr::Get(cpr::Url{url_});
    if (response.status_code != 200) {
        throw std::runtime_error("WebLoader: request failed (HTTP " + std::to_string(response.status_code) +
                                  "): " + url_);
    }

    core::Document document;
    document.content = detail::html_to_text(response.text);
    document.metadata = {{"source", url_}};
    return {document};
}

} // namespace langchain::rag
