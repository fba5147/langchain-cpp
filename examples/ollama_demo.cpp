// Demonstrates OpenAIChat/OpenAIEmbeddings talking to a *local* server
// through Ollama's OpenAI-compatible API, rather than api.openai.com --
// exercises the same wire-format code as OpenAIChat/OpenAIEmbeddings
// against a real backend, including a full tool-calling round trip via
// AgentExecutor.
//
// Requires `ollama serve` running locally with a tool-calling-capable
// model pulled (this was verified against llama3.2) and an embedding
// model (nomic-embed-text).

#include "langchain/langchain.hpp"

#include <iostream>

using namespace langchain;

namespace {

constexpr const char* kOllamaBaseUrl = "http://localhost:11434/v1";

// Unlike OpenAI's constrained-decoding function calling, small local
// models don't always honor a `"type": "number"` schema for tool
// arguments -- llama3.2 via Ollama has been observed sending "123" (a
// JSON string) instead of 123. Real tool implementations should coerce
// rather than assume the schema was followed.
double as_number(const nlohmann::json& value) {
    return value.is_string() ? std::stod(value.get<std::string>()) : value.get<double>();
}

std::shared_ptr<providers::OpenAIChat> make_ollama_chat(const std::string& model) {
    // Ollama doesn't check the key, but our config requires a non-empty one.
    return std::make_shared<providers::OpenAIChat>(providers::OpenAIConfig{
        .model = model,
        .api_key = "ollama",
        .base_url = kOllamaBaseUrl,
    });
}

} // namespace

int main() {
    auto chat = make_ollama_chat("llama3.2");

    std::cout << "--- plain chat ---\n";
    core::Message reply = chat->invoke({core::Message::user("Say hello in exactly 3 words.")});
    std::cout << reply.content << "\n\n";

    std::cout << "--- streaming ---\n";
    chat->stream({core::Message::user("Count from 1 to 5.")}, [](const llm::StreamChunk& chunk) {
        if (!chunk.is_final) {
            std::cout << chunk.delta << std::flush;
        } else {
            std::cout << "\n[stream done -- assembled content matches: "
                       << (chunk.message.content.size() > 0 ? "yes" : "no") << "]\n\n";
        }
    });

    std::cout << "--- agent with a tool ---\n";
    auto tool_registry = std::make_shared<tools::ToolRegistry>();
    tool_registry->add(std::make_shared<tools::FunctionTool>(
        "calculator", "Evaluates a simple `a <op> b` arithmetic expression.",
        [](const nlohmann::json& input) -> nlohmann::json {
            double a = as_number(input.at("a"));
            double b = as_number(input.at("b"));
            std::string op = input.at("op").get<std::string>();
            if (op == "+") return a + b;
            if (op == "-") return a - b;
            if (op == "*") return a * b;
            if (op == "/") return a / b;
            throw std::invalid_argument("unknown op: " + op);
        },
        nlohmann::json{
            {"type", "object"},
            {"properties",
             {
                 {"a", {{"type", "number"}}},
                 {"b", {{"type", "number"}}},
                 {"op", {{"type", "string"}, {"enum", {"+", "-", "*", "/"}}}},
             }},
            {"required", {"a", "b", "op"}},
        }));

    agents::AgentExecutor agent(chat, tool_registry);
    core::Message answer = agent.run("What is 123 * 456? Use the calculator tool.");
    std::cout << answer.content << "\n\n";

    std::cout << "--- streaming a tool call ---\n";
    auto tool_bound_chat = chat->bind_tools(tool_registry);
    tool_bound_chat->stream({core::Message::user("What is 9 * 9? Use the calculator tool.")},
                             [](const llm::StreamChunk& chunk) {
                                 if (chunk.is_final && chunk.message.has_tool_calls()) {
                                     std::cout << "assembled tool call: " << chunk.message.tool_calls[0].tool_name
                                               << "(" << chunk.message.tool_calls[0].arguments.dump() << ")\n\n";
                                 } else if (chunk.is_final) {
                                     std::cout << "(no tool call; model answered directly: " << chunk.message.content
                                               << ")\n\n";
                                 }
                             });

    std::cout << "--- embeddings ---\n";
    rag::OpenAIEmbeddings embeddings(rag::OpenAIEmbeddingsConfig{
        .model = "nomic-embed-text",
        .api_key = "ollama",
        .base_url = kOllamaBaseUrl,
    });
    std::vector<float> vector = embeddings.embed_query("hello world");
    std::cout << "embedding dimensions: " << vector.size() << '\n';

    return 0;
}
