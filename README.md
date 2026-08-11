# langchain-cpp

[![CI](https://github.com/fba5147/langchain-cpp/actions/workflows/ci.yml/badge.svg)](https://github.com/fba5147/langchain-cpp/actions/workflows/ci.yml)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](LICENSE)

An unofficial, native C++ take on [LangChain](https://github.com/langchain-ai/langchain): composable
building blocks for working with LLMs — chat models, prompt templates, output parsers, and (eventually)
tools, agents, RAG, and MCP. Not affiliated with or endorsed by the LangChain project.

The Python library is the reference for concepts and API shape, not a spec to mirror line-for-line — the
goal is an idiomatic C++ library, not a transliteration.

![A terminal recording showing examples/agent_demo giving an LLM a calculator tool and getting back "123 * 456 is 56088.", then examples/rag_demo retrieving the relevant document chunk and answering a question from it](docs/demo.gif)

## Status: v0.12.0 — core + six providers (streaming) + tools/structured output + agents + RAG + callbacks + caching + chat history + few-shot prompting + rate limiting

What exists today:

- **`Runnable<Input, Output>`** — the central abstraction, composed with `operator|`:
  - `RunnableSequence` — `prompt | model | parser`, mirroring LangChain's LCEL.
  - `RunnableLambda` — wraps a plain function as a Runnable for one-off glue steps.
  - `RunnablePassthrough<T>` — returns its input unchanged; used to carry the original input alongside
    a derived branch.
  - `RunnableParallel<Input, Output>` — runs named branches against the same input, collecting results
    into `unordered_map<string, Output>`. With `Output = std::string`, that's exactly `PromptValues`
    (see `prompts/prompt_template.hpp`), which is what makes a one-line
    `parallel | prompt | model | parser` RAG chain possible — see the RAG bullet below.
  - `RunnableBranch<Input, Output>` — routes to the first branch whose predicate matches the input.
- **`Message` / `MessageRole`** — value type for chat turns (system/user/assistant/tool), extended with
  `ToolCall` support: `Message::assistant_tool_calls(...)` for a reply that wants to call tools, and
  `Message::tool_result(call_id, content)` for the answer sent back.
- **`ChatModel`** — base interface (`Runnable<vector<Message>, Message>`), plus `stream()`: delivers
  incremental `StreamChunk`s (a text `delta`, then a final chunk with `is_final=true` and the fully
  assembled `Message`). The default synthesizes that single final chunk from `invoke()`, so every
  `ChatModel` supports `stream()` even without real incremental streaming; providers with it override
  the default. Implementations:
  - `MockChat` — canned response, function-driven response, or a scripted `vector<Message>` replayed
    one per call (repeating the last entry once exhausted) — no network. `stream()` simulates
    word-by-word delivery of whatever `invoke()` would have returned, so streaming consumers are
    testable offline too. Use it for tests and offline chain/agent development.
  - `OpenAIChat` — OpenAI Chat Completions API, including function-calling and real incremental SSE
    streaming (text deltas and fragmented tool-call arguments, both accumulated correctly — see
    `src/providers/openai/openai_wire_format.hpp`'s `OpenAiStreamParser`). Also works against any
    OpenAI-compatible server — Ollama, llama.cpp server, vLLM, LM Studio — by pointing `base_url` at
    it; **verified end-to-end against a live Ollama server, including streaming and streamed
    tool-calls**, see `examples/ollama_demo.cpp`.
  - `AzureOpenAIChat` — an Azure OpenAI deployment. Shares its wire-format and streaming code with
    `OpenAIChat` (the request/response body is identical); only the URL structure and auth header
    (`api-key`) differ.
  - `AnthropicChat` — Anthropic Messages API, including tool use (handles Anthropic's content-block
    array shape and its lack of a "tool" role under the hood).
  - `GeminiChat` — Google's Gemini API. A genuinely different wire format: roles are `user`/`model`
    (not `assistant`), the system prompt is a separate top-level field, and tool calls/results use
    `functionCall`/`functionResponse` parts with no call-id concept (correlation is by name, recovered
    internally — see `src/providers/google/gemini_wire_format.hpp`). Unit-tested against synthetic
    request/response JSON; not yet smoke-tested against the real endpoint (no `GOOGLE_API_KEY`
    available in the environment this was built in).
  - `GroqChat` / `MistralChat` / `DeepSeekChat` — thin presets over `OpenAIChat` for well-known
    OpenAI-compatible APIs (preset `base_url`/default model/API-key env var, nothing else). Any other
    OpenAI-compatible service works directly through `OpenAIChat` + `base_url`.
  - `ChatModel::bind_tools(registry)` returns a copy of the model that offers those tools on every
    subsequent `invoke()`; the default throws for providers that don't support it.
- **`PromptTemplate`** / **`ChatPromptTemplate`** — `{name}`-style placeholder substitution into a
  string or a rendered `vector<Message>`.
- **`FewShotPromptTemplate`** — the classic few-shot pattern: a prefix, each example rendered through
  `example_template` and joined by a separator, then a suffix rendered with the caller's own values.
  Examples come from a fixed list or, dynamically per input, from an `ExampleSelector` —
  `SemanticSimilarityExampleSelector` picks the `k` examples most similar to the input by embedding
  and cosine similarity (`core::cosine_similarity`, shared with `InMemoryVectorStore`'s retrieval).
- **`OutputFixingParser<T>`** — wraps a parser (e.g. `StructuredOutputParser<T>`); on a parse failure,
  re-prompts the *same* model with the invalid output and the error, asking it to fix the formatting,
  and retries up to `max_retries` times before rethrowing.
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
- **Callbacks** (`langchain::core::CallbackHandler`/`CallbackManager`, `langchain::callbacks`) —
  observability without touching every `Runnable`: `CallbackingChatModel`/`CallbackingTool` wrap any
  `ChatModel`/`Tool` to fire `on_llm_start/new_token/end/error` and `on_tool_start/end/error` events
  around calls, with zero changes to the wrapped object or to `AgentExecutor` — wrap once, get full
  agent-loop observability. `ConsoleCallbackHandler` is a ready-to-use handler that prints every event
  (mirrors LangChain's `StdOutCallbackHandler`); see `examples/callbacks_demo.cpp`.
- **Caching** (`langchain::llm`) — `CachingChatModel` wraps any `ChatModel` to skip it entirely on an
  identical request; `ChatModelCache` is a small pluggable interface (`InMemoryChatModelCache` ships
  out of the box). The cache key includes which tools are bound (via `bind_tools`), so a cached
  no-tools answer can't be served back for what's now effectively a different, tool-aware request.
  `stream()` on a cache hit delivers the cached answer as a single chunk instead of replaying it
  incrementally (there's nothing left to stream — the answer is already fully known); on a miss it
  delegates to the inner model's real `stream()` so incremental delivery isn't lost. See
  `examples/caching_demo.cpp`.
- **Chat message history** (`langchain::llm`) — `ChatMessageHistory` is a small interface
  (`messages()`/`add_message()`/`clear()`) with `InMemoryChatMessageHistory` and
  `FileChatMessageHistory` (JSON file, survives process restarts) implementations.
  `ChatModelWithHistory` wraps a `ChatModel` so each call only needs *this turn's* new message(s), not
  the whole conversation — history is loaded, the new turn appended, the model sees history + new
  turn, and the reply is appended back. Deliberately **not** a `ChatModel` itself: its `invoke()`
  means "just this turn," the opposite of the `ChatModel` contract used everywhere else (e.g.
  `AgentExecutor` always passes the full running conversation) — making it a `ChatModel` would invite
  exactly that mix-up. See `examples/chat_history_demo.cpp`, which really does persist across separate
  runs of the binary, not just within one process.
- **Rate limiting** (`langchain::llm`) — `RateLimiter` is a thread-safe token-bucket
  requests-per-second limiter (mirrors LangChain's `InMemoryRateLimiter`; limits request *frequency*,
  not LLM token/cost usage — that needs model-specific tokenization, out of scope here).
  `RateLimitedChatModel` wraps a `ChatModel` to throttle `invoke()`/`stream()` through it; share one
  `RateLimiter` across multiple wrapped models to throttle a whole application against one combined
  quota. `examples/rate_limit_demo.cpp` prints real elapsed times showing the throttling happen (2
  requests/sec lands calls roughly 0.5s apart after the first).
- **Multi-modal messages** (`core::ImageContent`/`Message::images`) — additive: `Message::content`
  keeps meaning plain text everywhere it already did. `Message::user_with_images(text, images)`
  attaches one or more images; `ImageContent::from_url(...)` wraps a URL, `ImageContent::from_file(path)`
  reads a local file, base64-encodes it, and guesses the media type from the extension.
  `OpenAIChat`/`AzureOpenAIChat` encode images as content-parts JSON — the only providers here that
  currently support them; `AnthropicChat`/`GeminiChat` throw a clear error rather than silently
  dropping an attached image. See `examples/multimodal_demo.cpp`.
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
  - `FormatDocumentsAsString` — joins retrieved `Document`s into one string (`Runnable<vector<Document>,
    string>`); `retriever | format_documents` composes with `RunnableParallel` into the one-line RAG
    chain below.

## Configuration (API keys)

Providers read their API key from an environment variable if `api_key` is left empty in their config
(see each provider's header for which one). For local development, `core::load_dotenv()` loads a
`.env` file from the current working directory into the process environment, without overriding
variables that are already set:

```bash
cp .env.example .env   # then fill in whichever keys you have
```

```cpp
int main() {
    core::load_dotenv();   // call this before constructing any provider
    // ...
}
```

Every example that can hit a real provider (`basic_chat`, `agent_demo`, `rag_demo`,
`more_providers_demo`) calls this at the top of `main()`, so dropping a `.env` file at the repo root
is enough to make them use real providers instead of falling back to `MockChat`/`MockEmbeddings` — no
need to `export` anything in your shell each session. `.env` is gitignored; never commit real keys.

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

Streaming — get incremental output instead of waiting for the whole reply:

```cpp
model->stream({core::Message::user("Count from 1 to 5.")}, [](const llm::StreamChunk& chunk) {
    if (!chunk.is_final) {
        std::cout << chunk.delta << std::flush;   // print as it arrives
    } else {
        std::cout << '\n';                        // chunk.message has the fully assembled reply
    }
});
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

Observability — wrap the model and each tool once, get the whole agent loop's activity printed:

```cpp
auto callback_manager = std::make_shared<core::CallbackManager>();
callback_manager->add_handler(std::make_shared<callbacks::ConsoleCallbackHandler>());

auto observed_model = std::make_shared<callbacks::CallbackingChatModel>(model, callback_manager);
auto observed_tool = std::make_shared<callbacks::CallbackingTool>(calculator, callback_manager);
// build a ToolRegistry from observed_tool and an AgentExecutor from observed_model as usual --
// AgentExecutor itself needs no changes.
```

Caching — skip the model entirely for a repeated request:

```cpp
auto cache = std::make_shared<llm::InMemoryChatModelCache>();
auto cached_model = std::make_shared<llm::CachingChatModel>(model, cache);

cached_model->invoke(messages); // calls the real model
cached_model->invoke(messages); // identical request -- served from cache, no call at all
```

Conversation memory — persisted across process restarts, not just within one run:

```cpp
auto history = std::make_shared<llm::FileChatMessageHistory>("chat_history.json");
llm::ChatModelWithHistory chat(model, history);

core::Message reply = chat.invoke({core::Message::user("Hello, do you remember me?")});
// next run of the same program, pointed at the same file, picks up where this left off
```

Few-shot prompting — with examples chosen dynamically per input:

```cpp
auto selector = std::make_shared<prompts::SemanticSimilarityExampleSelector>(
    embeddings, examples, /*input_key=*/"text", /*k=*/2);

prompts::FewShotPromptTemplate prompt(prompts::FewShotPromptTemplate::Config{
    .prefix = "Classify each sentence's topic.",
    .example_template = "Sentence: {text}\nTopic: {label}",
    .suffix = "Sentence: {text}\nTopic:",
    .example_selector = selector,   // omit and use `.examples = {...}` for a fixed set instead
});
std::string rendered = prompt.format({{"text", "What is RAII in C++?"}});
```

Rate limiting — throttle calls to stay under a provider's requests-per-second quota:

```cpp
auto limiter = std::make_shared<llm::RateLimiter>(/*requests_per_second=*/2.0, /*max_bucket_size=*/1.0);
auto limited_model = std::make_shared<llm::RateLimitedChatModel>(model, limiter);

limited_model->invoke(messages); // goes through immediately (bucket starts full)
limited_model->invoke(messages); // blocks briefly until the next token refills
```

Multi-modal messages — attach images alongside text (currently `OpenAIChat`/`AzureOpenAIChat` only;
`AnthropicChat`/`GeminiChat` throw rather than silently drop them):

```cpp
auto message = core::Message::user_with_images(
    "What's in this picture?", {core::ImageContent::from_file("cat.png")});
core::Message reply = model->invoke({message});
```

RAG — load, split, embed, index, retrieve, and answer, as one chain:

```cpp
auto documents = rag::TextLoader("docs/raii.txt").load();
auto chunks = rag::RecursiveCharacterTextSplitter().split_documents(documents);

auto store = std::make_shared<rag::InMemoryVectorStore>(std::make_shared<rag::OpenAIEmbeddings>());
store->add_documents(chunks);

auto retriever = store->as_retriever(/*k=*/3);
auto context_chain = retriever | std::make_shared<rag::FormatDocumentsAsString>();

core::RunnableParallel<std::string, std::string>::Branches branches;
branches["context"] = context_chain;
branches["question"] = std::make_shared<core::RunnablePassthrough<std::string>>();
auto parallel = std::make_shared<core::RunnableParallel<std::string, std::string>>(std::move(branches));

auto rag_chain = parallel | prompt | model | parser;   // Runnable<string, string>
std::string answer = rag_chain->invoke("What is RAII?");
```

That composes because `RunnableParallel<Input, std::string>`'s output type — `unordered_map<string,
string>` — is exactly `PromptValues`, so it flows straight into a `ChatPromptTemplate` with `{context}`
and `{question}` placeholders, no adapter needed.

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
./build/examples/more_providers_demo   # Gemini/Azure OpenAI/Groq -- each skips if its keys aren't set
./build/examples/callbacks_demo
./build/examples/caching_demo
./build/examples/chat_history_demo   # run it twice -- the message count grows across runs
./build/examples/few_shot_demo
./build/examples/rate_limit_demo   # prints real elapsed times -- takes a few seconds to run
./build/examples/multimodal_demo
OPENAI_API_KEY=sk-... ./build/examples/basic_chat
ANTHROPIC_API_KEY=sk-ant-... ./build/examples/basic_chat
GOOGLE_API_KEY=... ./build/examples/basic_chat
```

Or drop a `.env` file at the repo root (see [Configuration](#configuration-api-keys) above) and skip
the `KEY=... ` prefixes entirely.

### Verified against a real server (Ollama)

`OpenAIChat`/`OpenAIEmbeddings` were run against a local Ollama server (`llama3.2` +
`nomic-embed-text`), not just mocked — plain chat, the full tool-calling round trip (request →
`tool_calls` → executing the tool → feeding `tool_result` back → final answer), streaming (both plain
text deltas and a streamed tool-call, reassembled correctly from SSE fragments), and embeddings all
came back in the exact shape the code expects. One real finding from that testing: unlike OpenAI's
constrained-decoding function calling, small local models don't reliably honor a `"type": "number"`
JSON schema for tool arguments — llama3.2 sometimes sent `"123"` (a string) instead of `123`, in both
the non-streaming and streaming paths. The library's behavior was already correct here
(`FunctionTool::call` catches the resulting type error and reports it back to the model as a `Result`
error instead of crashing), but it's a good reminder that **tool implementations should coerce
loosely-typed arguments defensively** rather than assume a provider enforced its own schema —
`examples/ollama_demo.cpp`'s calculator does this.
`AnthropicChat`'s tool-calling wire format and `GeminiChat` (all of it) are written to spec but
haven't had the equivalent live smoke test in this environment — no Anthropic/Google credentials were
available. If you have one and hit a wire-format bug, that's exactly the gap this note is flagging;
please file an issue.

## Roadmap

Roughly in build order; each is its own milestone rather than all-at-once. Milestones 1-5 were broad
phases; from here on, milestone 6 (feature-parity gaps identified against the official Python/JS
libraries) is broken into lettered parts since it's too broad to land as one unit.

1. ~~Core types + `Runnable` + one provider~~ (v0.1.0)
2. ~~Tools + structured output parsing~~ (v0.2.0)
3. ~~Agents (LLM ↔ tool loop)~~ (v0.3.0)
4. ~~RAG stack: loaders, splitters, embeddings, vector stores, retrievers~~ (v0.4.0)
5. ~~More providers: Azure OpenAI, Gemini, OpenAI-compatible presets (Groq/Mistral/DeepSeek)~~ (v0.5.0)
6. Closing gaps against official LangChain (Python)/LangChain.js — see
   [this comparison](https://github.com/langchain-ai/langchain) for how these were identified:
   1. ~~**Runnable combinators**: `RunnableParallel`, `RunnablePassthrough`, `RunnableBranch` — unlocks
      a one-line `retriever | prompt | model | parser` RAG chain~~ (v0.6.0)
   2. ~~**Streaming**: `ChatModel::stream()` returning incremental `StreamChunk`s~~ (this release) —
      went with a callback (`StreamCallback`) over C++20 coroutines: simpler to implement correctly,
      easier to unit-test (a plain lambda collecting chunks), and works today without needing a
      hand-rolled generator type (`std::generator` is C++23). A coroutine-based `for (auto& chunk :
      model->stream(...))` wrapper could still be layered on top later without changing this
      interface. Real incremental SSE streaming is implemented for `OpenAIChat`/`AzureOpenAIChat`
      (including reassembling tool-call argument fragments split across chunks) and verified live
      against Ollama; other providers use the default (one synthesized final chunk).
   3. ~~**Callbacks/tracing**: lightweight observability hooks~~ (this release) — no generic
      `on_chain_start/end` for arbitrary `Runnable`s (that would mean threading a config/callbacks
      parameter through every `invoke()`, which is invasive); instead `CallbackingChatModel`/
      `CallbackingTool` decorators wrap a `ChatModel`/`Tool` to fire `on_llm_*`/`on_tool_*` events,
      composing with `AgentExecutor` and `bind_tools()` with no changes to either. No
      LangSmith-equivalent tracing backend — just the hook points and a console printer for now.
   4. ~~**Caching**: a pluggable LLM response cache~~ (v0.9.0) — `CachingChatModel` decorator (same
      pattern as callbacks) + `ChatModelCache` interface + `InMemoryChatModelCache`. Cache key is
      derived from model name, conversation, and bound tools, so a no-tools answer can't get served
      back for what's now a tool-aware request.
   5. ~~**Chat message history**: a persisted conversation store abstraction~~ (this release) —
      `ChatMessageHistory` interface, `InMemoryChatMessageHistory`, and `FileChatMessageHistory`
      (JSON file, survives restarts), plus a `ChatModelWithHistory` wrapper. Note that official
      "Memory" classes are themselves largely superseded by LangGraph state, so this was lower
      priority than it might look — implemented mainly because it composes cleanly with everything
      else already here, not because it's where the ecosystem is investing.
   6. ~~**Prompting/parsing extras**: few-shot prompt templates, example selectors, an
      output-fixing/retry parser~~ (this release) — `FewShotPromptTemplate`, `ExampleSelector` +
      `SemanticSimilarityExampleSelector` (reuses `core::cosine_similarity`, newly extracted from
      `InMemoryVectorStore` for this second use case), and `OutputFixingParser<T>`.
   7. ~~**Rate limiting**: a token/request rate limiter wrapper for `ChatModel`~~ (this release) —
      `RateLimiter` (thread-safe token-bucket requests-per-second limiter) + `RateLimitedChatModel`.
      Limits request frequency, not LLM token/cost usage — that needs model-specific tokenization,
      out of scope here.
   8. ~~**Multi-modal messages**: image content blocks in `Message`~~ (this release) —
      `core::ImageContent`/`Message::images`, additive alongside the existing text-only `.content`.
      `OpenAIChat`/`AzureOpenAIChat` encode them; `AnthropicChat`/`GeminiChat` throw a clear error
      instead of silently dropping them, since neither's wire-format support is implemented yet. Audio
      content blocks are still out of scope — no provider here has an audio API to target yet.
   9. **MCP client/server support** — confirmed as an official package
      (`@langchain/mcp-adapters` in LangChain.js) during the comparison, so this is a real gap, not
      speculative scope; likely the most differentiated piece relative to other C++ LLM libraries.
      Local inference via a direct llama.cpp/GGUF binding (no server in between) is related — Ollama
      already works today through `OpenAIChat` + `base_url`, see `examples/ollama_demo.cpp`.
   10. **Integration breadth**: more vector stores (FAISS, Qdrant, pgvector), document loaders (PDF,
       CSV, web/HTML), and embeddings providers — the official libs' partner-package model, at a much
       smaller scale.
7. Live smoke tests for `AnthropicChat` tool-calling and `GeminiChat` against their real endpoints

## Contributing

Contributions are welcome — see [CONTRIBUTING.md](CONTRIBUTING.md) for build/test instructions, coding
conventions, and where the roadmap most needs help. Please also read the
[Code of Conduct](CODE_OF_CONDUCT.md).

## Benchmarks

[benchmarks/RESULTS.md](benchmarks/RESULTS.md) has cold-start and mocked-chain-throughput comparisons
against Python LangChain (`langchain-core`) and TypeScript LangChain (`@langchain/core`), a recorded
run, and how to reproduce it with `benchmarks/run_all.sh`. Full LLM-call latency isn't benchmarked —
it's dominated by network/API time, identical across languages, so it wouldn't measure anything about
the frameworks themselves. One-line summary of the recorded run: C++ cold-starts ~6–23x faster and
pushes ~5–90x the throughput of TypeScript/Python respectively on a no-network mocked chain — see the
caveats in that file (notably, that run's C++ binary wasn't even native arm64) before citing the exact
numbers.

## License

[MIT](LICENSE)
