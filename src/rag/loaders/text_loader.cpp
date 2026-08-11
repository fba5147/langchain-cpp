#include "langchain/rag/loaders/text_loader.hpp"

#include <fstream>
#include <sstream>
#include <stdexcept>

namespace langchain::rag {

TextLoader::TextLoader(std::string path) : path_(std::move(path)) {}

std::string TextLoader::read_file() const {
    std::ifstream file(path_);
    if (!file) {
        throw std::runtime_error("TextLoader: could not open file: " + path_);
    }
    std::ostringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

std::vector<core::Document> TextLoader::load() {
    core::Document document;
    document.content = read_file();
    document.metadata = {{"source", path_}};
    return {document};
}

} // namespace langchain::rag
