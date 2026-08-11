#include "langchain/core/message.hpp"

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

} // namespace langchain::core
