#pragma once

#include <string>
#include <utility>
#include <variant>

namespace langchain::core {

struct Error {
    std::string message;
};

// A minimal Result<T>, for call sites (tool execution, agent steps, ...)
// that want to report failure without throwing.
template <typename T>
class Result {
public:
    Result(T value) : state_(std::move(value)) {}
    Result(Error error) : state_(std::move(error)) {}

    bool ok() const { return std::holds_alternative<T>(state_); }
    explicit operator bool() const { return ok(); }

    const T& value() const { return std::get<T>(state_); }
    T& value() { return std::get<T>(state_); }

    const Error& error() const { return std::get<Error>(state_); }

private:
    std::variant<T, Error> state_;
};

} // namespace langchain::core
