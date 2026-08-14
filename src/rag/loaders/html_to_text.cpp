#include "html_to_text.hpp"

#include <algorithm>
#include <cctype>
#include <utility>
#include <vector>

namespace langchain::rag::detail {

namespace {

std::string to_lower(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char c) { return std::tolower(c); });
    return value;
}

// Removes every <tag ...>...</tag> block, including its own content --
// script/style bodies aren't meant to become visible text.
std::string remove_elements_entirely(const std::string& html, const std::string& tag_name) {
    std::string lower = to_lower(html);
    std::string open_tag = "<" + tag_name;
    std::string close_tag = "</" + tag_name;

    std::string result;
    result.reserve(html.size());
    std::size_t pos = 0;
    while (pos < html.size()) {
        std::size_t open_pos = lower.find(open_tag, pos);
        if (open_pos == std::string::npos) {
            result.append(html, pos, std::string::npos);
            break;
        }
        result.append(html, pos, open_pos - pos);

        std::size_t close_pos = lower.find(close_tag, open_pos);
        if (close_pos == std::string::npos) {
            break; // unterminated -- drop the rest rather than emit a half-open element
        }
        std::size_t close_end = lower.find('>', close_pos);
        pos = close_end == std::string::npos ? html.size() : close_end + 1;
    }
    return result;
}

std::string decode_entities(std::string text) {
    static const std::vector<std::pair<std::string, std::string>> kEntities = {
        {"&nbsp;", " "}, {"&amp;", "&"}, {"&lt;", "<"}, {"&gt;", ">"}, {"&quot;", "\""}, {"&apos;", "'"}, {"&#39;", "'"},
    };
    for (const auto& entry : kEntities) {
        std::size_t pos = 0;
        while ((pos = text.find(entry.first, pos)) != std::string::npos) {
            text.replace(pos, entry.first.length(), entry.second);
            pos += entry.second.length();
        }
    }
    return text;
}

// Closing tags (and <br>, which has no meaningful closing form) insert
// the line break: the *end* of a block is where a real reader's eye
// would move to a new line.
bool is_closing_block_boundary(const std::string& lower_tag) {
    static const std::vector<std::string> kPrefixes = {"br", "/p",  "/div", "/li", "/h1",
                                                         "/h2", "/h3", "/h4",  "/h5", "/h6",
                                                         "/tr"};
    for (const auto& prefix : kPrefixes) {
        if (lower_tag.rfind(prefix, 0) == 0) {
            return true;
        }
    }
    return false;
}

// Opening tags of the same block elements insert nothing: whatever
// preceded them (either nothing, or the previous sibling's closing tag)
// already provided the necessary break, so adding a space here would
// leave a stray " " right after that newline.
bool is_opening_block_tag(const std::string& lower_tag) {
    static const std::vector<std::string> kPrefixes = {"p", "div", "li", "h1", "h2", "h3", "h4", "h5", "h6", "tr"};
    for (const auto& prefix : kPrefixes) {
        if (lower_tag.rfind(prefix, 0) == 0) {
            return true;
        }
    }
    return false;
}

} // namespace

std::string html_to_text(const std::string& html) {
    std::string without_scripts = remove_elements_entirely(html, "script");
    std::string without_styles = remove_elements_entirely(without_scripts, "style");

    // Strip tags, inserting a newline at block-ish boundaries and a plain
    // space otherwise, so words from adjacent elements don't run together.
    std::string text;
    text.reserve(without_styles.size());
    bool in_tag = false;
    std::string current_tag;
    for (char c : without_styles) {
        if (c == '<') {
            in_tag = true;
            current_tag.clear();
            continue;
        }
        if (c == '>') {
            in_tag = false;
            std::string lower_tag = to_lower(current_tag);
            if (is_closing_block_boundary(lower_tag)) {
                text += '\n';
            } else if (!is_opening_block_tag(lower_tag)) {
                text += ' ';
            }
            continue;
        }
        if (in_tag) {
            current_tag += c;
            continue;
        }
        text += c;
    }

    text = decode_entities(text);

    // Collapse whitespace: runs of spaces/tabs -> one space, runs of
    // newlines -> at most two (a paragraph break), trim the ends.
    std::string collapsed;
    collapsed.reserve(text.size());
    int consecutive_newlines = 0;
    bool last_was_space = false;
    for (char c : text) {
        if (c == '\n') {
            ++consecutive_newlines;
            last_was_space = false;
            continue;
        }
        if (consecutive_newlines > 0) {
            collapsed.append(consecutive_newlines > 1 ? 2 : 1, '\n');
            consecutive_newlines = 0;
        }
        if (c == ' ' || c == '\t' || c == '\r') {
            if (!last_was_space) {
                collapsed += ' ';
                last_was_space = true;
            }
            continue;
        }
        collapsed += c;
        last_was_space = false;
    }
    if (consecutive_newlines > 0) {
        collapsed.append(consecutive_newlines > 1 ? 2 : 1, '\n');
    }

    std::size_t start = collapsed.find_first_not_of(" \t\n");
    if (start == std::string::npos) {
        return "";
    }
    std::size_t end = collapsed.find_last_not_of(" \t\n");
    return collapsed.substr(start, end - start + 1);
}

} // namespace langchain::rag::detail
