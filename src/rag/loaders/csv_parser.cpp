#include "csv_parser.hpp"

namespace langchain::rag::detail {

std::vector<std::vector<std::string>> parse_csv(const std::string& content, char delimiter) {
    std::vector<std::vector<std::string>> rows;
    std::vector<std::string> current_row;
    std::string field;
    bool in_quotes = false;
    bool field_started_with_quote = false;

    std::size_t i = 0;
    while (i < content.size()) {
        char c = content[i];

        if (in_quotes) {
            if (c == '"') {
                if (i + 1 < content.size() && content[i + 1] == '"') {
                    field += '"'; // escaped quote
                    i += 2;
                    continue;
                }
                in_quotes = false;
                i += 1;
                continue;
            }
            field += c;
            i += 1;
            continue;
        }

        if (c == '"' && field.empty() && !field_started_with_quote) {
            in_quotes = true;
            field_started_with_quote = true;
            i += 1;
            continue;
        }

        if (c == delimiter) {
            current_row.push_back(field);
            field.clear();
            field_started_with_quote = false;
            i += 1;
            continue;
        }

        if (c == '\r') {
            i += 1; // swallow; a following '\n' (or end of file) ends the row
            continue;
        }

        if (c == '\n') {
            current_row.push_back(field);
            rows.push_back(current_row);
            current_row.clear();
            field.clear();
            field_started_with_quote = false;
            i += 1;
            continue;
        }

        field += c;
        i += 1;
    }

    if (!field.empty() || !current_row.empty()) {
        current_row.push_back(field);
        rows.push_back(current_row);
    }

    return rows;
}

} // namespace langchain::rag::detail
