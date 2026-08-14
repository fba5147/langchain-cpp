#pragma once

// A minimal, dependency-free HTML-to-text conversion: strips tags,
// drops <script>/<style> elements entirely (including their content, not
// just the tags), decodes the handful of HTML entities that show up in
// ordinary text, and collapses excess whitespace left behind by removed
// tags. This is NOT a real HTML parser (no DOM, no malformed-markup
// recovery beyond "ignore unmatched brackets") -- good enough to turn a
// typical article/doc page into readable plain text for embedding, not a
// general-purpose HTML processing tool. Kept pure and separately
// testable, same reasoning as the CSV parser.

#include <string>

namespace langchain::rag::detail {

std::string html_to_text(const std::string& html);

} // namespace langchain::rag::detail
