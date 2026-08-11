#pragma once

#include "langchain/llm/chat_model.hpp"

#include <cstddef>
#include <functional>
#include <string>
#include <vector>

namespace langchain::providers {

// A ChatModel that never hits the network: either always returns a fixed
// response, delegates to a user-supplied function, or replays a scripted
// sequence of Messages (advancing one per invoke(), repeating the last
// entry once exhausted). The scripted form is what makes AgentExecutor
// testable without a real provider: script a tool-call turn followed by a
// final answer.
class MockChat : public llm::ChatModel {
public:
    using ResponseFn = std::function<std::string(const std::vector<core::Message>&)>;

    explicit MockChat(std::string fixed_response = "Hello from MockChat");
    explicit MockChat(ResponseFn fn);
    explicit MockChat(std::vector<core::Message> scripted_responses);

    core::Message invoke(const std::vector<core::Message>& messages) override;
    std::string model_name() const override { return "mock-chat"; }

    // Tools are ignored: MockChat's behavior is entirely driven by its
    // fixed response / function / script, not by what tools are offered.
    std::shared_ptr<llm::ChatModel> bind_tools(std::shared_ptr<tools::ToolRegistry> registry) override;

private:
    ResponseFn fn_;
    std::vector<core::Message> scripted_responses_;
    std::size_t next_scripted_ = 0;
};

} // namespace langchain::providers
