# langchain-cpp

An unofficial, native C++ take on [LangChain](https://github.com/langchain-ai/langchain): composable
building blocks for working with LLMs — chat models, prompt templates, output parsers, and (eventually)
tools, agents, RAG, and MCP. Not affiliated with or endorsed by the LangChain project.

The Python library is the reference for concepts and API shape, not a spec to mirror line-for-line — the
goal is an idiomatic C++ library, not a transliteration.

## Status: v0.4.0 — core + one provider + tools/structured output + agents + RAG

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
- **`Document`** / **`Result<T>`** — small core value types; `Result<T>` is used by `Tool::call` to
  report failure without throwing.
- **RAG stack** (`langchain::rag`):
  - `DocumentLoader` / `TextLoader` / `MarkdownLoader` — load a file into one or more `Document`s.
  - `RecursiveCharacterTextSplitter` — splits text into `chunk_size`-bounded pieces, trying paragraph
    breaks, then lines, then words, then raw characters, and carrying `chunk_overlap` characters of
    trailing context into the next chunk.
  - `Embeddings` — base interface (`embed_query` / `embed_documents`), with `MockEmbeddings`
    (deterministic hash-based bag-of-words, no network) and `OpenAIEmbeddings` (OpenAI's embeddings
    API).
  - `VectorStore` / `InMemoryVectorStore` — add documents, brute-force cosine-similarity search;
    `store->as_retriever(k)` wraps it as a `Retriever`.
  - `Retriever` — a `Runnable<string, vector<Document>>`, so it composes into a chain like any other
    Runnable.

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

RAG — load, split, embed, index, and retrieve:

```cpp
auto documents = rag::TextLoader("docs/raii.txt").load();
auto chunks = rag::RecursiveCharacterTextSplitter().split_documents(documents);

auto store = std::make_shared<rag::InMemoryVectorStore>(std::make_shared<rag::OpenAIEmbeddings>());
store->add_documents(chunks);

auto retriever = store->as_retriever(/*k=*/3);
std::vector<core::Document> relevant = retriever->invoke("What is RAII?");
```

`Retriever::invoke` returns `vector<Document>`, not a `string`, so it doesn't (yet) pipe directly into
a `ChatPromptTemplate` with `operator|` — that needs a `RunnableParallel`/passthrough combinator this
milestone didn't build. `examples/rag_demo.cpp` shows the current pattern: call the retriever
explicitly, then feed its output into the prompt's `context` variable.

See `examples/mock_chain.cpp` and `examples/structured_output.cpp` for fully offline versions of the
first two snippets above (no API key needed), `examples/basic_chat.cpp` for a real provider call,
`examples/tools_demo.cpp` for defining a `Tool` and rendering its function-calling schema,
`examples/agent_demo.cpp` for the full agent loop (scripted offline, or live with an API key set),
`examples/rag_demo.cpp` for the full RAG pipeline (offline with `MockEmbeddings`, or live with
`OPENAI_API_KEY` set), and `examples/ollama_demo.cpp` for `OpenAIChat`/`OpenAIEmbeddings` pointed at a
local [Ollama](https://ollama.com) server instead of `api.openai.com` (chat, tool-calling agent loop,
and embeddings, all verified end-to-end against a real `llama3.2` + `nomic-embed-text` server — see
note below on local-model tool-calling reliability).

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
./build/examples/rag_demo
./build/examples/ollama_demo   # requires `ollama serve` + `ollama pull llama3.2 nomic-embed-text`
OPENAI_API_KEY=sk-... ./build/examples/basic_chat
ANTHROPIC_API_KEY=sk-ant-... ./build/examples/basic_chat
```

### Verified against a real server (Ollama)

`OpenAIChat`/`OpenAIEmbeddings` were run against a local Ollama server (`llama3.2` +
`nomic-embed-text`), not just mocked — plain chat, the full tool-calling round trip (request →
`tool_calls` → executing the tool → feeding `tool_result` back → final answer), and embeddings all
came back in the exact shape the code expects. One real finding from that testing: unlike OpenAI's
constrained-decoding function calling, small local models don't reliably honor a `"type": "number"`
JSON schema for tool arguments — llama3.2 sometimes sent `"123"` (a string) instead of `123`. The
library's behavior was already correct here (`FunctionTool::call` catches the resulting type error and
reports it back to the model as a `Result` error instead of crashing), but it's a good reminder that
**tool implementations should coerce loosely-typed arguments defensively** rather than assume a
provider enforced its own schema — `examples/ollama_demo.cpp`'s calculator does this.
`AnthropicChat`'s tool-calling wire format is written to the same spec but hasn't had the equivalent
live smoke test in this environment.

## Roadmap

Roughly in build order; each is its own milestone rather than all-at-once:

1. ~~Core types + `Runnable` + one provider~~ (v0.1.0)
2. ~~Tools + structured output parsing~~ (v0.2.0)
3. ~~Agents (LLM ↔ tool loop)~~ (v0.3.0)
4. ~~RAG stack: loaders, splitters, embeddings, vector stores, retrievers~~ (this release)
5. MCP client/server support
6. Local inference (llama.cpp, Ollama)
7. Streaming, async, cancellation, retries, callbacks, tracing, and a `RunnableParallel`/passthrough
   combinator (needed for a one-line `retriever | prompt | model | parser` RAG chain)

## License

TBD.
