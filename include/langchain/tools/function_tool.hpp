#pragma once

#include "langchain/tools/tool.hpp"

#include <functional>

namespace langchain::tools {

// Wraps a plain function as a Tool — the simplest way to give an agent a
// new capability. Exceptions thrown by the function are caught and turned
// into a Result error rather than propagating.
class FunctionTool : public Tool {
public:
    using Fn = std::function<nlohmann::json(const nlohmann::json&)>;

    FunctionTool(std::string name, std::string description, Fn fn,
                 nlohmann::json parameters_schema = nlohmann::json::object());

    std::string name() const override { return name_; }
    std::string description() const override { return description_; }
    nlohmann::json parameters_schema() const override { return parameters_schema_; }
    core::Result<nlohmann::json> call(const nlohmann::json& input) override;

private:
    std::string name_;
    std::string description_;
    nlohmann::json parameters_schema_;
    Fn fn_;
};

} // namespace langchain::tools
