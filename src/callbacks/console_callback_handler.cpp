#include "langchain/callbacks/console_callback_handler.hpp"

namespace langchain::callbacks {

ConsoleCallbackHandler::ConsoleCallbackHandler(std::ostream& out) : out_(out) {}

void ConsoleCallbackHandler::on_llm_start(const core::LlmStartEvent& event) {
    out_ << "[llm start] " << event.model_name << " (" << event.messages.size() << " messages)\n";
}

void ConsoleCallbackHandler::on_llm_new_token(const core::LlmNewTokenEvent& event) {
    out_ << event.delta << std::flush;
}

void ConsoleCallbackHandler::on_llm_end(const core::LlmEndEvent& event) {
    out_ << "[llm end] " << event.model_name << ": "
         << (event.result.has_tool_calls() ? "(requested a tool call)" : event.result.content) << '\n';
}

void ConsoleCallbackHandler::on_llm_error(const core::LlmErrorEvent& event) {
    out_ << "[llm error] " << event.model_name << ": " << event.error << '\n';
}

void ConsoleCallbackHandler::on_tool_start(const core::ToolStartEvent& event) {
    out_ << "[tool start] " << event.tool_name << "(" << event.input.dump() << ")\n";
}

void ConsoleCallbackHandler::on_tool_end(const core::ToolEndEvent& event) {
    out_ << "[tool end] " << event.tool_name << " -> " << event.output.dump() << '\n';
}

void ConsoleCallbackHandler::on_tool_error(const core::ToolErrorEvent& event) {
    out_ << "[tool error] " << event.tool_name << ": " << event.error << '\n';
}

} // namespace langchain::callbacks
