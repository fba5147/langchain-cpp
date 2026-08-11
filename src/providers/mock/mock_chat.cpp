#include "langchain/providers/mock/mock_chat.hpp"

#include <algorithm>
#include <sstream>

namespace langchain::providers {

MockChat::MockChat(std::string fixed_response)
    : fn_([response = std::move(fixed_response)](const std::vector<core::Message>&) { return response; }) {}

MockChat::MockChat(ResponseFn fn) : fn_(std::move(fn)) {}

MockChat::MockChat(std::vector<core::Message> scripted_responses)
    : scripted_responses_(std::move(scripted_responses)) {}

core::Message MockChat::invoke(const std::vector<core::Message>& messages) {
    if (!scripted_responses_.empty()) {
        std::size_t index = std::min(next_scripted_, scripted_responses_.size() - 1);
        ++next_scripted_;
        return scripted_responses_[index];
    }
    return core::Message::assistant(fn_(messages));
}

std::shared_ptr<llm::ChatModel> MockChat::bind_tools(std::shared_ptr<tools::ToolRegistry>) {
    return std::make_shared<MockChat>(*this);
}

void MockChat::stream(const std::vector<core::Message>& messages, const StreamCallback& on_chunk) {
    core::Message result = invoke(messages);

    if (!result.content.empty()) {
        std::istringstream stream(result.content);
        std::string word;
        bool first = true;
        while (stream >> word) {
            on_chunk(llm::StreamChunk{first ? word : " " + word, false, {}});
            first = false;
        }
    }

    on_chunk(llm::StreamChunk{"", true, result});
}

} // namespace langchain::providers
