#pragma once

#include "langchain/llm/chat_model.hpp"

#include <functional>
#include <string>
#include <vector>

namespace langchain::providers {

// A ChatModel that never hits the network: either always returns a fixed
// response, or delegates to a user-supplied function. Useful for tests,
// examples, and offline development of chains.
class MockChat : public llm::ChatModel {
public:
    using ResponseFn = std::function<std::string(const std::vector<core::Message>&)>;

    explicit MockChat(std::string fixed_response = "Hello from MockChat");
    explicit MockChat(ResponseFn fn);

    core::Message invoke(const std::vector<core::Message>& messages) override;
    std::string model_name() const override { return "mock-chat"; }

private:
    ResponseFn fn_;
};

} // namespace langchain::providers
