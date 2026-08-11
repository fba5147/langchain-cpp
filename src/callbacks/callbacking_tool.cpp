#include "langchain/callbacks/callbacking_tool.hpp"

namespace langchain::callbacks {

CallbackingTool::CallbackingTool(std::shared_ptr<tools::Tool> inner, std::shared_ptr<core::CallbackManager> callbacks)
    : inner_(std::move(inner)), callbacks_(std::move(callbacks)) {}

core::Result<nlohmann::json> CallbackingTool::call(const nlohmann::json& input) {
    callbacks_->on_tool_start({inner_->name(), input});

    core::Result<nlohmann::json> result = inner_->call(input);
    if (result.ok()) {
        callbacks_->on_tool_end({inner_->name(), result.value()});
    } else {
        callbacks_->on_tool_error({inner_->name(), result.error().message});
    }
    return result;
}

} // namespace langchain::callbacks
