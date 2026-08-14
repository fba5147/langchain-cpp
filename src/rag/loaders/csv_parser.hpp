#pragma once

// A minimal RFC 4180-style CSV parser: handles quoted fields (including
// embedded delimiters, embedded newlines, and "" as an escaped quote),
// kept pure and separately testable rather than folded into CsvLoader's
// file-reading code, since the quoting rules are the part actually worth
// getting right and unit-testing directly.

#include <string>
#include <vector>

namespace langchain::rag::detail {

std::vector<std::vector<std::string>> parse_csv(const std::string& content, char delimiter = ',');

} // namespace langchain::rag::detail
