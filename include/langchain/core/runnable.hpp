#pragma once

#include <functional>
#include <future>
#include <memory>
#include <type_traits>
#include <vector>

namespace langchain::core {

// The central abstraction: anything that turns an Input into an Output.
// Concrete Runnables (PromptTemplate, ChatModel, OutputParser, ...) are
// composed with operator| into a RunnableSequence, mirroring LangChain's
// LCEL `prompt | model | parser` chains.
template <typename Input, typename Output>
class Runnable {
public:
    using InputType = Input;
    using OutputType = Output;

    virtual ~Runnable() = default;

    virtual Output invoke(const Input& input) = 0;

    virtual std::vector<Output> batch(const std::vector<Input>& inputs) {
        std::vector<Output> results;
        results.reserve(inputs.size());
        for (const auto& input : inputs) {
            results.push_back(invoke(input));
        }
        return results;
    }

    virtual std::future<Output> ainvoke(const Input& input) {
        return std::async(std::launch::async, [this, input]() { return invoke(input); });
    }
};

template <typename Input, typename Mid, typename Output>
class RunnableSequence : public Runnable<Input, Output> {
public:
    RunnableSequence(std::shared_ptr<Runnable<Input, Mid>> first, std::shared_ptr<Runnable<Mid, Output>> second)
        : first_(std::move(first)), second_(std::move(second)) {}

    Output invoke(const Input& input) override { return second_->invoke(first_->invoke(input)); }

private:
    std::shared_ptr<Runnable<Input, Mid>> first_;
    std::shared_ptr<Runnable<Mid, Output>> second_;
};

// Wraps a plain function as a Runnable, for one-off glue steps in a chain.
template <typename Input, typename Output>
class RunnableLambda : public Runnable<Input, Output> {
public:
    using Fn = std::function<Output(const Input&)>;

    explicit RunnableLambda(Fn fn) : fn_(std::move(fn)) {}

    Output invoke(const Input& input) override { return fn_(input); }

private:
    Fn fn_;
};

template <typename Input, typename Output>
std::shared_ptr<RunnableLambda<Input, Output>> make_runnable_lambda(std::function<Output(const Input&)> fn) {
    return std::make_shared<RunnableLambda<Input, Output>>(std::move(fn));
}

// Generic `first | second` composition. Works for any two shared_ptrs to
// Runnable subclasses (not just Runnable<Input,Output> itself) by reading
// the InputType/OutputType aliases inherited from the base template, so
// `prompt | model | parser` works directly on concrete provider/parser
// types without the caller upcasting anything.
template <typename First, typename Second>
auto operator|(std::shared_ptr<First> first, std::shared_ptr<Second> second)
    -> std::shared_ptr<Runnable<typename First::InputType, typename Second::OutputType>> {
    static_assert(std::is_same_v<typename First::OutputType, typename Second::InputType>,
                  "Runnable chain type mismatch: output of the first step must match the input of the next");

    using Input = typename First::InputType;
    using Mid = typename First::OutputType;
    using Output = typename Second::OutputType;

    return std::make_shared<RunnableSequence<Input, Mid, Output>>(
        std::static_pointer_cast<Runnable<Input, Mid>>(std::move(first)),
        std::static_pointer_cast<Runnable<Mid, Output>>(std::move(second)));
}

} // namespace langchain::core
