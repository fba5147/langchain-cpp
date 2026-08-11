// Demonstrates the three Part-F prompting/parsing extras:
//   1. FewShotPromptTemplate with a fixed set of examples.
//   2. The same template, but with examples chosen dynamically per input
//      via SemanticSimilarityExampleSelector.
//   3. OutputFixingParser recovering from a malformed structured-output
//      reply by re-prompting the model with the parse error.
//
// Fully offline -- MockEmbeddings/MockChat throughout, no API key needed.

#include "langchain/langchain.hpp"

#include <iostream>

using namespace langchain;

namespace {
struct Person {
    std::string name;
    int age;

    NLOHMANN_DEFINE_TYPE_INTRUSIVE(Person, name, age)
};
} // namespace

int main() {
    std::cout << "--- few-shot prompt template (fixed examples) ---\n";
    prompts::FewShotPromptTemplate fixed_prompt(prompts::FewShotPromptTemplate::Config{
        .prefix = "Classify the sentiment of each sentence as positive or negative.",
        .example_template = "Sentence: {text}\nSentiment: {label}",
        .suffix = "Sentence: {text}\nSentiment:",
        .examples = {
            {{"text", "I love this library."}, {"label", "positive"}},
            {{"text", "This is frustrating to use."}, {"label", "negative"}},
        },
    });
    std::cout << fixed_prompt.format({{"text", "Building this was a lot of fun."}}) << "\n\n";

    std::cout << "--- few-shot prompt template (semantic example selector) ---\n";
    auto embeddings = std::make_shared<rag::MockEmbeddings>();
    auto selector = std::make_shared<prompts::SemanticSimilarityExampleSelector>(
        embeddings,
        std::vector<prompts::PromptValues>{
            {{"text", "RAII ties resource lifetime to object lifetime."}, {"label", "language feature"}},
            {{"text", "Vector databases store embeddings for nearest-neighbor search."},
             {"label", "data infrastructure"}},
            {{"text", "Quantum computers use superconducting qubits."}, {"label", "hardware"}},
        },
        "text", /*k=*/1);

    prompts::FewShotPromptTemplate dynamic_prompt(prompts::FewShotPromptTemplate::Config{
        .prefix = "Classify each sentence's topic.",
        .example_template = "Sentence: {text}\nTopic: {label}",
        .suffix = "Sentence: {text}\nTopic:",
        .example_selector = selector,
    });
    // Should pull in the RAII example, not the vector-database or quantum ones.
    std::cout << dynamic_prompt.format({{"text", "What is RAII in C++?"}}) << "\n\n";

    std::cout << "--- output-fixing parser ---\n";
    auto structured_parser = std::make_shared<parsers::StructuredOutputParser<Person>>();
    auto fixer_model = std::make_shared<providers::MockChat>(std::vector<core::Message>{
        core::Message::assistant(R"({"name": "Ada Lovelace", "age": 36})"),
    });
    parsers::OutputFixingParser<Person> fixing_parser(structured_parser, fixer_model);

    Person person = fixing_parser.invoke(core::Message::assistant("Sure! The person is Ada Lovelace, age 36."));
    std::cout << "Recovered from malformed output via a re-prompt: " << person.name << ", age " << person.age
              << '\n';

    return 0;
}
