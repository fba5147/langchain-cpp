#pragma once

#include "langchain/core/callbacks.hpp"
#include "langchain/tools/tool.hpp"

#include <memory>

namespace langchain::callbacks {

// Wraps any Tool to fire CallbackManager events around call(), without
// changing the wrapped tool at all.
class CallbackingTool : public tools::Tool {
public:
    CallbackingTool(std::shared_ptr<tools::Tool> inner, std::shared_ptr<core::CallbackManager> callbacks);

    std::string name() const override { return inner_->name(); }
    std::string description() const override { return inner_->description(); }
    nlohmann::json parameters_schema() const override { return inner_->parameters_schema(); }
    core::Result<nlohmann::json> call(const nlohmann::json& input) override;

private:
    std::shared_ptr<tools::Tool> inner_;
    std::shared_ptr<core::CallbackManager> callbacks_;
};

} // namespace langchain::callbacks
