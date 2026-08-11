#include "langchain/core/dotenv.hpp"

#include <cstdlib>
#include <fstream>

namespace langchain::core {

namespace {

std::string trim(const std::string& s) {
    std::size_t start = s.find_first_not_of(" \t\r\n");
    if (start == std::string::npos) {
        return "";
    }
    std::size_t end = s.find_last_not_of(" \t\r\n");
    return s.substr(start, end - start + 1);
}

std::string strip_quotes(const std::string& s) {
    if (s.size() >= 2) {
        char first = s.front();
        char last = s.back();
        if ((first == '"' && last == '"') || (first == '\'' && last == '\'')) {
            return s.substr(1, s.size() - 2);
        }
    }
    return s;
}

} // namespace

void load_dotenv(const std::string& path) {
    std::ifstream file(path);
    if (!file) {
        return;
    }

    std::string line;
    while (std::getline(file, line)) {
        std::string trimmed = trim(line);
        if (trimmed.empty() || trimmed[0] == '#') {
            continue;
        }

        std::size_t eq = trimmed.find('=');
        if (eq == std::string::npos) {
            continue;
        }

        std::string key = trim(trimmed.substr(0, eq));
        std::string value = strip_quotes(trim(trimmed.substr(eq + 1)));
        if (key.empty()) {
            continue;
        }

        setenv(key.c_str(), value.c_str(), /*overwrite=*/0);
    }
}

} // namespace langchain::core
