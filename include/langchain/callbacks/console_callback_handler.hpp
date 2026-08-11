#pragma once

#include "langchain/core/callbacks.hpp"

#include <iostream>
#include <ostream>

namespace langchain::callbacks {

// Prints every event to an ostream (std::cout by default) -- a
// ready-to-use handler for quickly seeing what a chain/agent is doing,
// mirroring LangChain's StdOutCallbackHandler.
class ConsoleCallbackHandler : public core::CallbackHandler {
public:
    explicit ConsoleCallbackHandler(std::ostream& out = std::cout);

    void on_llm_start(const core::LlmStartEvent& event) override;
    void on_llm_new_token(const core::LlmNewTokenEvent& event) override;
    void on_llm_end(const core::LlmEndEvent& event) override;
    void on_llm_error(const core::LlmErrorEvent& event) override;

    void on_tool_start(const core::ToolStartEvent& event) override;
    void on_tool_end(const core::ToolEndEvent& event) override;
    void on_tool_error(const core::ToolErrorEvent& event) override;

private:
    std::ostream& out_;
};

} // namespace langchain::callbacks
