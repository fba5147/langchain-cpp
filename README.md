# langchain-cpp

An unofficial, native C++ take on [LangChain](https://github.com/langchain-ai/langchain): composable
building blocks for working with LLMs — chat models, prompt templates, output parsers, and (eventually)
tools, agents, RAG, and MCP. Not affiliated with or endorsed by the LangChain project.

The Python library is the reference for concepts and API shape, not a spec to mirror line-for-line — the
goal is an idiomatic C++ library, not a transliteration.

## Status: v0.1.0 — core + one provider

What exists today:

- **`Runnable<Input, Output>`** — the central abstraction, composed with `operator|` into a
  `RunnableSequence`, mirroring LangChain's LCEL `prompt | model | parser` chains.
- **`Message` / `MessageRole`** — plain value type for chat turns (system/user/assistant/tool).
- **`ChatModel`** — base interface (`Runnable<vector<Message>, Message>`), with three implementations:
  - `MockChat` — canned or function-driven responses, no network. Use it for tests and offline chain
    development.
  - `OpenAIChat` — OpenAI Chat Completions API (also works against any OpenAI-compatible server —
    Ollama, llama.cpp server, vLLM, LM Studio — by pointing `base_url` at it).
  - `AnthropicChat` — Anthropic Messages API.
- **`PromptTemplate`** / **`ChatPromptTemplate`** — `{name}`-style placeholder substitution into a
  string or a rendered `vector<Message>`.
- **`StrOutputParser`** — pulls the text back out of a `Message`.
- **`Document`** / **`Result<T>`** — small core value types reserved for later phases (RAG documents,
  fallible tool/agent results).

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

See `examples/mock_chain.cpp` for a fully offline version of this (no API key needed), and
`examples/basic_chat.cpp` for a real provider call.

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
OPENAI_API_KEY=sk-... ./build/examples/basic_chat
ANTHROPIC_API_KEY=sk-ant-... ./build/examples/basic_chat
```

## Roadmap

Roughly in build order; each is its own milestone rather than all-at-once:

1. ~~Core types + `Runnable` + one provider~~ (this release)
2. Tools + structured output parsing
3. Agents (LLM ↔ tool loop)
4. RAG stack: loaders, splitters, embeddings, vector stores, retrievers
5. MCP client/server support
6. Local inference (llama.cpp, Ollama)
7. Streaming, async, cancellation, retries, callbacks, tracing

## License

TBD.
