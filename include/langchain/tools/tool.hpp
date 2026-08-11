#pragma once

#include "langchain/core/result.hpp"

#include <nlohmann/json.hpp>
#include <string>

namespace langchain::tools {

// Something an agent can call: a name and description an LLM picks from
// during tool selection, a JSON-schema description of its expected input,
// and a JSON-in/JSON-out call interface.
class Tool {
public:
    virtual ~Tool() = default;

    virtual std::string name() const = 0;
    virtual std::string description() const = 0;
    virtual nlohmann::json parameters_schema() const = 0;
    virtual core::Result<nlohmann::json> call(const nlohmann::json& input) = 0;
};

} // namespace langchain::tools
