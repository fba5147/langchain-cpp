#pragma once

#include <string>

namespace langchain::core {

// Loads KEY=VALUE lines from `path` into the process environment, for keys
// not already set -- a real exported environment variable always wins
// over the file. A missing file is not an error (most environments won't
// have one). Lines starting with '#' and blank lines are skipped; values
// may be wrapped in matching single or double quotes.
//
// Call this once near the top of main() before constructing any provider
// that reads an API key from the environment.
void load_dotenv(const std::string& path = ".env");

} // namespace langchain::core
