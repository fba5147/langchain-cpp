# langchain-cpp

An unofficial, native C++ take on [LangChain](https://github.com/langchain-ai/langchain): composable
building blocks for working with LLMs — chat models, prompt templates, output parsers, and (eventually)
tools, agents, RAG, and MCP. Not affiliated with or endorsed by the LangChain project.

The Python library is the reference for concepts and API shape, not a spec to mirror line-for-line — the
goal is an idiomatic C++ library, not a transliteration.

## Status: v0.3.0 — core + one provider + tools/structured output + agents

What exists today:

- **`Runnable<Input, Output>`** — the central abstraction, composed with `operator|` into a
  `RunnableSequence`, mirroring LangChain's LCEL `prompt | model | parser` chains.
- **`Message` / `MessageRole`** — value type for chat turns (system/user/assistant/tool), extended with
  `ToolCall` support: `Message::assistant_tool_calls(...)` for a reply that wants to call tools, and
  `Message::tool_result(call_id, content)` for the answer sent back.
- **`ChatModel`** — base interface (`Runnable<vector<Message>, Message>`), with three implementations:
  - `MockChat` — canned response, function-driven response, or a scripted `vector<Message>` replayed
    one per call (repeating the last entry once exhausted) — no network. Use it for tests and offline
    chain/agent development.
  - `OpenAIChat` — OpenAI Chat Completions API, including function-calling (also works against any
    OpenAI-compatible server — Ollama, llama.cpp server, vLLM, LM Studio — by pointing `base_url` at
    it).
  - `AnthropicChat` — Anthropic Messages API, including tool use (handles Anthropic's content-block
    array shape and its lack of a "tool" role under the hood).
  - `ChatModel::bind_tools(registry)` returns a copy of the model that offers those tools on every
    subsequent `invoke()`; the default throws for providers that don't support it.
- **`PromptTemplate`** / **`ChatPromptTemplate`** — `{name}`-style placeholder substitution into a
  string or a rendered `vector<Message>`.
- **`StrOutputParser`** — pulls the text back out of a `Message`.
- **`JsonOutputParser`** / **`StructuredOutputParser<T>`** — parses a `Message`'s content as JSON
  (tolerating a ` ```json ` fence), or straight into a C++ type `T` via nlohmann::json's ADL
  `to_json`/`from_json`. Where Python LangChain resolves the target schema at runtime, here `T` is a
  compile-time parameter, so `chain->invoke(...)` returns a real `Person`, not a dict you cast.
- **`Tool`** / **`FunctionTool`** / **`ToolRegistry`** — a callable an agent can pick from
  (name/description/JSON-schema parameters), a way to wrap a plain function as one, and a registry
  that renders both OpenAI- and Anthropic-style function-calling JSON.
- **`AgentExecutor`** — runs the LLM ↔ tool loop: binds the registry to the model, executes whatever
  tools a reply asks for, feeds the results back as `tool_result` messages, and repeats until a plain
  answer comes back or `AgentConfig::max_steps` is exceeded.
- **`Document`** / **`Result<T>`** — small core value types; `Document` is reserved for the RAG phase,
  `Result<T>` is used by `Tool::call` to report failure without throwing.

## Example

```cpp
#include "langchain/langchain.hpp"

using namespace langchain;

auto prompt = std::make_shared<prompts::ChatPromptTemplate>(
    std::vector<std::pair<core::MessageRole, std::string>>{
        {core::MessageRole::System, "You are an expert in {language}."},
        {core::MessageRole::User, "Explain {topic}."},
    });

auto model = std::make_shared<providers::OpenAIChat>(providers::OpenAIConfig{
    .model = "gpt-4o-mini",
});

auto parser = std::make_shared<parsers::StrOutputParser>();

auto chain = prompt | model | parser;

std::string answer = chain->invoke({{"language", "C++"}, {"topic", "RAII"}});
```

Structured output — parse straight into a C++ type instead of a string:

```cpp
struct Person {
    std::string name;
    int age;
};
NLOHMANN_DEFINE_TYPE_INTRUSIVE(Person, name, age)

auto chain = prompt | model | std::make_shared<parsers::StructuredOutputParser<Person>>();
Person person = chain->invoke({{"name", "Ada Lovelace"}, {"age", "36"}});
```

Agents — give a model tools and let it decide when to call them:

```cpp
auto tools = std::make_shared<tools::ToolRegistry>();
tools->add(std::make_shared<tools::FunctionTool>(
    "calculator", "Evaluates a simple `a <op> b` arithmetic expression.",
    [](const nlohmann::json& input) -> nlohmann::json { /* ... */ },
    nlohmann::json{/* JSON schema for {a, b, op} */}));

agents::AgentExecutor agent(model, tools);
core::Message answer = agent.run("What is 123 * 456?");
```

See `examples/mock_chain.cpp` and `examples/structured_output.cpp` for fully offline versions of the
first two snippets above (no API key needed), `examples/basic_chat.cpp` for a real provider call,
`examples/tools_demo.cpp` for defining a `Tool` and rendering its function-calling schema, and
`examples/agent_demo.cpp` for the full agent loop (scripted offline, or live with an API key set).

## Building

Requires a C++20 compiler, CMake 3.21+, and [vcpkg](https://github.com/microsoft/vcpkg).

```bash
export VCPKG_ROOT=/path/to/vcpkg
cmake --preset default
cmake --build build
ctest --test-dir build
```

Run the examples:

```bash
./build/examples/mock_chain
./build/examples/structured_output
./build/examples/tools_demo
./build/examples/agent_demo
OPENAI_API_KEY=sk-... ./build/examples/basic_chat
ANTHROPIC_API_KEY=sk-ant-... ./build/examples/basic_chat
```

## Roadmap

Roughly in build order; each is its own milestone rather than all-at-once:

1. ~~Core types + `Runnable` + one provider~~ (v0.1.0)
2. ~~Tools + structured output parsing~~ (v0.2.0)
3. ~~Agents (LLM ↔ tool loop)~~ (this release)
4. RAG stack: loaders, splitters, embeddings, vector stores, retrievers
5. MCP client/server support
6. Local inference (llama.cpp, Ollama)
7. Streaming, async, cancellation, retries, callbacks, tracing

## License

TBD.
