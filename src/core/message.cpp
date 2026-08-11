#include "langchain/core/message.hpp"

#include <cctype>
#include <fstream>
#include <stdexcept>

namespace langchain::core {

std::string to_api_role(MessageRole role) {
    switch (role) {
        case MessageRole::System:
            return "system";
        case MessageRole::User:
            return "user";
        case MessageRole::Assistant:
            return "assistant";
        case MessageRole::Tool:
            return "tool";
    }
    throw std::invalid_argument("to_api_role: unknown MessageRole");
}

namespace {

std::string base64_encode(const std::vector<unsigned char>& bytes) {
    static const char* table = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

    std::string result;
    result.reserve(((bytes.size() + 2) / 3) * 4);

    std::size_t i = 0;
    while (i + 3 <= bytes.size()) {
        unsigned int chunk = (static_cast<unsigned int>(bytes[i]) << 16) |
                              (static_cast<unsigned int>(bytes[i + 1]) << 8) | static_cast<unsigned int>(bytes[i + 2]);
        result += table[(chunk >> 18) & 0x3F];
        result += table[(chunk >> 12) & 0x3F];
        result += table[(chunk >> 6) & 0x3F];
        result += table[chunk & 0x3F];
        i += 3;
    }

    std::size_t remaining = bytes.size() - i;
    if (remaining == 1) {
        unsigned int chunk = static_cast<unsigned int>(bytes[i]) << 16;
        result += table[(chunk >> 18) & 0x3F];
        result += table[(chunk >> 12) & 0x3F];
        result += "==";
    } else if (remaining == 2) {
        unsigned int chunk = (static_cast<unsigned int>(bytes[i]) << 16) | (static_cast<unsigned int>(bytes[i + 1]) << 8);
        result += table[(chunk >> 18) & 0x3F];
        result += table[(chunk >> 12) & 0x3F];
        result += table[(chunk >> 6) & 0x3F];
        result += "=";
    }

    return result;
}

std::string guess_media_type(const std::string& path) {
    std::size_t dot = path.find_last_of('.');
    if (dot == std::string::npos) {
        return "";
    }
    std::string ext = path.substr(dot + 1);
    for (char& c : ext) {
        c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    }
    if (ext == "png") {
        return "image/png";
    }
    if (ext == "jpg" || ext == "jpeg") {
        return "image/jpeg";
    }
    if (ext == "gif") {
        return "image/gif";
    }
    if (ext == "webp") {
        return "image/webp";
    }
    return "";
}

} // namespace

ImageContent ImageContent::from_file(const std::string& path, std::string media_type) {
    std::ifstream file(path, std::ios::binary);
    if (!file) {
        throw std::runtime_error("ImageContent::from_file: could not open " + path);
    }

    std::vector<unsigned char> bytes(
        (std::istreambuf_iterator<char>(file)),
        std::istreambuf_iterator<char>());

    if (media_type.empty()) {
        media_type = guess_media_type(path);
        if (media_type.empty()) {
            throw std::runtime_error("ImageContent::from_file: could not guess a media type for " + path +
                                      "; pass one explicitly");
        }
    }

    return ImageContent{ImageSourceType::Base64, base64_encode(bytes), std::move(media_type)};
}

} // namespace langchain::core
