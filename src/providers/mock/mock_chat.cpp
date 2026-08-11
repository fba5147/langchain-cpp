#include "langchain/providers/mock/mock_chat.hpp"

namespace langchain::providers {

MockChat::MockChat(std::string fixed_response)
    : fn_([response = std::move(fixed_response)](const std::vector<core::Message>&) { return response; }) {}

MockChat::MockChat(ResponseFn fn) : fn_(std::move(fn)) {}

core::Message MockChat::invoke(const std::vector<core::Message>& messages) {
    return core::Message::assistant(fn_(messages));
}

} // namespace langchain::providers
