#include "langchain/tools/function_tool.hpp"

#include <exception>

namespace langchain::tools {

FunctionTool::FunctionTool(std::string name, std::string description, Fn fn, nlohmann::json parameters_schema)
    : name_(std::move(name)),
      description_(std::move(description)),
      parameters_schema_(std::move(parameters_schema)),
      fn_(std::move(fn)) {}

core::Result<nlohmann::json> FunctionTool::call(const nlohmann::json& input) {
    try {
        return fn_(input);
    } catch (const std::exception& e) {
        return core::Error{e.what()};
    }
}

} // namespace langchain::tools
