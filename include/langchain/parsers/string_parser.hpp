#pragma once

#include "langchain/core/message.hpp"
#include "langchain/core/runnable.hpp"

namespace langchain::parsers {

// Extracts the raw text content from a Message, e.g. as the tail end of
// `prompt | model | StrOutputParser`.
class StrOutputParser : public core::Runnable<core::Message, std::string> {
public:
    std::string invoke(const core::Message& message) override { return message.content; }
};

} // namespace langchain::parsers
