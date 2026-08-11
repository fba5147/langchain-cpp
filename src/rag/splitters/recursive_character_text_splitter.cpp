#include "langchain/rag/splitters/recursive_character_text_splitter.hpp"

#include <utility>

namespace langchain::rag {

namespace {

std::vector<std::string> split_string(const std::string& text, const std::string& separator) {
    if (separator.empty()) {
        std::vector<std::string> chars;
        chars.reserve(text.size());
        for (char c : text) {
            chars.push_back(std::string(1, c));
        }
        return chars;
    }

    std::vector<std::string> pieces;
    std::size_t pos = 0;
    while (true) {
        std::size_t next = text.find(separator, pos);
        if (next == std::string::npos) {
            pieces.push_back(text.substr(pos));
            break;
        }
        pieces.push_back(text.substr(pos, next - pos));
        pos = next + separator.size();
    }
    return pieces;
}

std::string join(const std::vector<std::string>& pieces, const std::string& separator) {
    std::string result;
    for (std::size_t i = 0; i < pieces.size(); ++i) {
        if (i > 0) {
            result += separator;
        }
        result += pieces[i];
    }
    return result;
}

} // namespace

RecursiveCharacterTextSplitter::RecursiveCharacterTextSplitter() : RecursiveCharacterTextSplitter(Config{}) {}

RecursiveCharacterTextSplitter::RecursiveCharacterTextSplitter(Config config) : config_(std::move(config)) {}

std::vector<std::string> RecursiveCharacterTextSplitter::split_text(const std::string& text) const {
    if (text.empty()) {
        return {};
    }
    return split(text, 0);
}

std::vector<core::Document> RecursiveCharacterTextSplitter::split_documents(
    const std::vector<core::Document>& documents) const {
    std::vector<core::Document> result;
    for (const auto& document : documents) {
        for (auto& chunk : split_text(document.content)) {
            core::Document piece;
            piece.content = std::move(chunk);
            piece.metadata = document.metadata;
            result.push_back(std::move(piece));
        }
    }
    return result;
}

std::vector<std::string> RecursiveCharacterTextSplitter::split(const std::string& text,
                                                                 std::size_t separator_index) const {
    bool at_last_separator = separator_index + 1 >= config_.separators.size();
    const std::string& separator = config_.separators[separator_index];

    std::vector<std::string> pieces = split_string(text, separator);

    std::vector<std::string> good_pieces;
    for (auto& piece : pieces) {
        if (piece.size() <= config_.chunk_size || at_last_separator) {
            good_pieces.push_back(std::move(piece));
        } else {
            auto sub_pieces = split(piece, separator_index + 1);
            good_pieces.insert(good_pieces.end(), sub_pieces.begin(), sub_pieces.end());
        }
    }

    return merge_splits(good_pieces, separator);
}

std::vector<std::string> RecursiveCharacterTextSplitter::merge_splits(const std::vector<std::string>& pieces,
                                                                        const std::string& separator) const {
    std::vector<std::string> chunks;
    std::vector<std::string> current;
    std::size_t current_len = 0;
    std::size_t sep_len = separator.size();

    for (const auto& piece : pieces) {
        std::size_t added_len = current.empty() ? piece.size() : piece.size() + sep_len;

        if (!current.empty() && current_len + added_len > config_.chunk_size) {
            chunks.push_back(join(current, separator));

            std::vector<std::string> overlap;
            std::size_t overlap_len = 0;
            for (auto it = current.rbegin(); it != current.rend(); ++it) {
                std::size_t len = it->size() + (overlap.empty() ? 0 : sep_len);
                if (overlap_len + len > config_.chunk_overlap) {
                    break;
                }
                overlap.insert(overlap.begin(), *it);
                overlap_len += len;
            }
            current = std::move(overlap);
            current_len = overlap_len;
            added_len = current.empty() ? piece.size() : piece.size() + sep_len;
        }

        current.push_back(piece);
        current_len += added_len;
    }

    if (!current.empty()) {
        chunks.push_back(join(current, separator));
    }

    return chunks;
}

} // namespace langchain::rag
