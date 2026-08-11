#pragma once

#include "langchain/core/message.hpp"
#include "langchain/core/runnable.hpp"
#include "langchain/parsers/json_parser.hpp"

namespace langchain::parsers {

// Parses a Message's JSON content directly into T via nlohmann::json's
// ADL to_json/from_json, e.g. a T declared with
// NLOHMANN_DEFINE_TYPE_INTRUSIVE(T, field1, field2, ...). Where Python
// LangChain resolves structured output at runtime via a schema, here the
// target type is a compile-time parameter, so a chain built with
// `StructuredOutputParser<Person>` statically returns a Person.
template <typename T>
class StructuredOutputParser : public core::Runnable<core::Message, T> {
public:
    T invoke(const core::Message& message) override { return extract_json(message.content).template get<T>(); }
};

} // namespace langchain::parsers
