# Changelog

All notable changes to this project are documented here. Format loosely follows
[Keep a Changelog](https://keepachangelog.com/en/1.1.0/); this project does not yet follow strict
semantic versioning (pre-1.0, breaking changes can land in a minor bump).

## [0.13.0] — Multi-modal messages

Part 8 of the roadmap-item-6 gap list.

- `core::ImageContent`/`ImageSourceType` and `Message::images` — additive, backward-compatible: every
  existing `Message` and every place that reads `.content` as plain text is unaffected.
  `Message::user_with_images(text, images)` builds one; `ImageContent::from_url(...)` wraps a URL as-is,
  `ImageContent::from_file(path, media_type = "")` reads a local file, base64-encodes it, and guesses
  the media type from the extension (`.png`/`.jpg`/`.jpeg`/`.gif`/`.webp`) if none is given.
- `OpenAIChat`/`AzureOpenAIChat` (shared wire format) encode images as OpenAI's content-parts array
  (`image_url` with either the URL as-is or a `data:<media_type>;base64,<data>` URL) — the only
  providers here that currently support them.
- `AnthropicChat`/`GeminiChat` throw a clear `std::runtime_error` if given a message with images,
  rather than silently dropping them — both providers can support images, but the wire-format work
  hasn't been done yet, and silent data loss is worse than a loud error. Anthropic's conversion logic
  was extracted into a new testable `detail::` module (`anthropic_wire_format.hpp/.cpp`, mirroring the
  existing Gemini one) specifically so this throw — and the pre-existing conversion shapes — could be
  unit-tested without a live `ANTHROPIC_API_KEY`.
- Two blind spots that a naive `Message.images` addition would have silently broken, fixed proactively:
  `CachingChatModel`'s cache key now includes image data, so two requests differing only by an attached
  image no longer collide; `FileChatMessageHistory` now round-trips images through its JSON persistence.
- `examples/multimodal_demo.cpp` — builds a URL image and a locally-encoded file image, runs one through
  a `MockChat`, and shows `AnthropicChat`/`GeminiChat` throwing immediately (no network call needed,
  since the throw happens during request-building) when given an image.
- 24 new tests. One caught a real, pre-existing bug: `parse_openai_message` used
  `json::value("content", "")`, which only substitutes its default for a *missing* key — real OpenAI
  responses send `"content": null` (not absent) alongside `tool_calls`, which threw a
  `json.exception.type_error` instead of falling back to an empty string. Fixed with an explicit
  present-but-null check.

## [0.12.0] — Rate limiting

Part 7 of the roadmap-item-6 gap list.

- `llm::RateLimiter` — thread-safe requests-per-second token-bucket limiter (mirrors LangChain's
  `InMemoryRateLimiter`); a single instance can be shared across multiple wrapped models to throttle
  a whole application against one combined quota. Limits request frequency, not LLM token/cost usage.
- `llm::RateLimitedChatModel` — decorator (same pattern as callbacks/caching) that calls
  `RateLimiter::acquire()` before `invoke()`/`stream()`.
- `examples/rate_limit_demo.cpp` — verified running: with 2 requests/sec and a bucket size of 1, the
  first call landed at t≈0 and the next three landed at t≈0.51s/1.01s/1.51s — real, visible throttling.
- 8 new tests. The two that exercise real blocking use generous timing tolerances (checking "did it
  block roughly the right ballpark," not an exact duration) to stay reliable under CI load rather than
  introducing a fake-clock abstraction just for testability; confirmed stable across repeated runs.

## [0.11.0] — Few-shot prompting and output-fixing parser

Part 6 of the roadmap-item-6 gap list.

- `core::cosine_similarity` — extracted from `InMemoryVectorStore` (which now calls it instead of its
  own private copy) once a second real use case (example selection, below) justified sharing it.
- `prompts::FewShotPromptTemplate` — prefix + examples (rendered through `example_template`, joined by
  a separator) + suffix, the classic few-shot pattern.
- `prompts::ExampleSelector` interface + `prompts::SemanticSimilarityExampleSelector` — picks the `k`
  examples most similar to the input (by embedding + `core::cosine_similarity`) instead of always
  using the same fixed set; plugs into `FewShotPromptTemplate::Config::example_selector`.
- `parsers::OutputFixingParser<T>` — wraps a parser; on failure, re-prompts the same model with the
  invalid output and the parse error, retrying up to `max_retries` times before rethrowing.
- `examples/few_shot_demo.cpp` — verified running: the semantic selector correctly pulls in the RAII
  example (not the unrelated vector-database/quantum-computing ones) for a RAII-related query, and the
  output-fixing parser recovers a `Person` from a deliberately unparseable first reply.
- 15 new tests.

## [0.10.0] — Chat message history

Part 5 of the roadmap-item-6 gap list.

- `llm::ChatMessageHistory` — small interface (`messages()`/`add_message()`/`clear()`), plus
  `llm::InMemoryChatMessageHistory` and `llm::FileChatMessageHistory` (JSON file on disk, loaded on
  construction, rewritten on every change — survives process restarts, verified by actually running
  `examples/chat_history_demo.cpp` twice and watching the message count grow across runs).
- `llm::ChatModelWithHistory` — wraps a `ChatModel` so each `invoke()` only needs *this turn's* new
  message(s), not the whole conversation: loads history, appends the new turn, calls the inner model
  with history + new turn, appends the reply, returns it. Deliberately not a `ChatModel` subclass —
  its `invoke()` means "just this turn," which is the opposite of the `ChatModel` contract used
  everywhere else (`AgentExecutor` always passes the full running conversation); making it a
  `ChatModel` would invite exactly that mix-up.
- 7 new tests, including a round-trip test that persists a tool-call/tool-result exchange to a file
  and reloads it via a fresh `FileChatMessageHistory` instance (simulating a process restart).

## [0.9.0] — Caching

Part 4 of the roadmap-item-6 gap list.

- `llm::ChatModelCache` — small pluggable interface (`get`/`put`), plus `llm::InMemoryChatModelCache`.
- `llm::CachingChatModel` — decorator (same pattern as the callbacks decorators) that skips the inner
  model entirely on a cache hit. Cache key is derived from model name, the full conversation, and a
  signature of any bound tools (reusing `ToolRegistry::to_openai_tools_json()` as a deterministic,
  distinguishing signature) — this matters: without it, a cached no-tools answer could get served back
  for what's now a tool-aware request. `stream()` on a hit delivers the cached answer as one final
  chunk; on a miss it delegates to the inner model's real `stream()` so incremental delivery for
  providers that support it isn't lost, then stores the assembled result.
- `examples/caching_demo.cpp` — a call-counting model demonstrates the effect directly (identical
  question returns the same numbered response without bumping the counter; a different question does).
- 8 new tests. One test's first draft asserted on the wrong object (`CountingChatModel::bind_tools`
  returns a *copy*, so its counter isn't the same object the test held a reference to) and had to be
  rewritten to check the cache's entry count directly — a reminder that a failing test is sometimes
  telling you about the test, not the code under test; both are worth checking before "fixing" either.

## [0.8.0] — Callbacks/tracing

Part 3 of the roadmap-item-6 gap list.

- `core::CallbackHandler`/`CallbackManager` — event structs (`LlmStartEvent`, `LlmNewTokenEvent`,
  `LlmEndEvent`, `LlmErrorEvent`, `ToolStartEvent`, `ToolEndEvent`, `ToolErrorEvent`) and a handler
  base class with no-op defaults; `CallbackManager` fans out to multiple handlers and swallows a
  misbehaving handler's exceptions rather than letting them break the chain being observed.
- `callbacks::CallbackingChatModel` / `callbacks::CallbackingTool` — decorators that wrap any
  `ChatModel`/`Tool` to fire events around `invoke()`/`stream()`/`call()`, with no changes to the
  wrapped object or to `AgentExecutor`. Chosen over threading a callbacks parameter through every
  `Runnable::invoke()`, which would have touched every existing Runnable.
- `callbacks::ConsoleCallbackHandler` — prints every event; a ready-to-use handler (mirrors
  LangChain's `StdOutCallbackHandler`).
- `examples/callbacks_demo.cpp` — wraps a scripted `MockChat` and a `FunctionTool`, runs them through
  an unmodified `AgentExecutor`, and shows the full LLM/tool activity trace.
- 9 new tests, including one that caught a real bug in the first draft of
  `CallbackingChatModel::stream()`: it fired callback events but never forwarded chunks to the
  caller's own callback.

## [0.7.0] — Streaming

Part 2 of the roadmap-item-6 gap list.

- `llm::StreamChunk` and `ChatModel::stream(messages, on_chunk)` — callback-based (not
  coroutine-based; see README roadmap item 6.2 for the reasoning). Default implementation synthesizes
  a single final chunk from `invoke()`, so every existing `ChatModel` supports `stream()` correctly
  without changes.
- `OpenAIChat`/`AzureOpenAIChat` override `stream()` with real incremental SSE streaming, via a new
  `cpr::Session` + `WriteCallback`-based path and a shared `detail::OpenAiStreamParser`
  (`src/providers/openai/openai_wire_format.hpp`) that accumulates text deltas and reassembles
  tool-call `arguments` fragments split across chunks (verified against both the fragmented-argument
  case synthetically and Ollama's whole-argument-in-one-chunk case live).
- `MockChat::stream()` simulates word-by-word delivery of whatever `invoke()` would return, so
  streaming consumers are unit-testable offline.
- Verified live against Ollama: plain-text streaming and a streamed tool call, both reassembling
  correctly (see the updated `examples/ollama_demo.cpp`).

## [0.6.0] — Runnable combinators

Part 1 of the roadmap-item-6 gap list (identified by comparing against official LangChain
Python/LangChain.js).

- `core::RunnablePassthrough<T>`, `core::RunnableParallel<Input, Output>`,
  `core::RunnableBranch<Input, Output>` — added to `core/runnable.hpp` alongside the existing
  `RunnableSequence`/`RunnableLambda`.
- `rag::FormatDocumentsAsString` — `Runnable<vector<Document>, string>`, joins retrieved documents.
- `examples/rag_demo.cpp` rewritten: the retrieval + generation pipeline is now one
  `parallel | prompt | model | parser` chain (previously required manually calling the retriever and
  feeding its output into the prompt). Works because `RunnableParallel<Input, std::string>`'s output
  type is exactly `PromptValues`.

## [0.5.0] — More providers

- `core::load_dotenv(path = ".env")` — loads `KEY=VALUE` lines into the process environment for keys
  not already set. `.env.example` documents every variable the providers/examples read; `.env` itself
  is gitignored. Wired into every example that can hit a real provider.
- `AzureOpenAIChat` — talks to an Azure OpenAI deployment; shares wire-format code with `OpenAIChat`
  (moved into `providers::detail` so both can use it), differing only in URL structure and auth header.
- `GeminiChat` — Google's Gemini API. A genuinely different wire format (`user`/`model` roles, a
  separate system-instruction field, `functionCall`/`functionResponse` parts with no call-id concept);
  the pure conversion logic is unit-tested (`tests/test_gemini_wire_format.cpp`) since no
  `GOOGLE_API_KEY` was available to smoke-test against the real endpoint.
- `GroqChat` / `MistralChat` / `DeepSeekChat` — presets over `OpenAIChat` for well-known
  OpenAI-compatible APIs.
- `ToolRegistry::to_gemini_tools_json()`.
- `examples/more_providers_demo.cpp`.

## [0.4.1] — Open-source scaffolding, benchmarks, Ollama verification

- Open-source project scaffolding: LICENSE (MIT), CONTRIBUTING.md, CODE_OF_CONDUCT.md, SECURITY.md,
  GitHub issue/PR templates, CI workflow.
- `benchmarks/`: cold-start and mocked-chain-throughput comparisons against Python LangChain
  (`langchain-core`) and TypeScript LangChain (`@langchain/core`).
- `examples/ollama_demo.cpp`: `OpenAIChat`/`OpenAIEmbeddings` verified end-to-end against a local
  Ollama server (chat, tool-calling agent loop, embeddings) — caught and fixed a real bug in
  `MockEmbeddings`' tokenizer (punctuation-attached words never hash-matched) along the way.
- `docs/demo.gif`: a recorded terminal demo embedded in the README.

## [0.4.0] — RAG stack

- `rag::TextLoader` / `rag::MarkdownLoader` — load a file into one or more `Document`s.
- `rag::RecursiveCharacterTextSplitter` — chunk text with configurable `chunk_size`/`chunk_overlap`,
  trying paragraph/line/word/character separators in turn.
- `rag::Embeddings` base interface, with `MockEmbeddings` (deterministic, no network) and
  `OpenAIEmbeddings`.
- `rag::VectorStore` / `rag::InMemoryVectorStore` (brute-force cosine similarity) and
  `rag::Retriever` (`Runnable<string, vector<Document>>`), reachable via `VectorStore::as_retriever`.
- `examples/rag_demo.cpp`.

## [0.3.0] — Agents

- `core::Message` extended with `ToolCall`, `Message::assistant_tool_calls(...)`, and
  `Message::tool_result(call_id, content)` (replacing the ambiguous `Message::tool(content)`).
- `llm::ChatModel::bind_tools(registry)` — returns a tool-aware copy of a model; default throws for
  providers that don't support it.
- `OpenAIChat` and `AnthropicChat` both implement full tool-calling wire formats, including
  Anthropic's content-block array shape and its lack of a "tool" role.
- `MockChat` gained a scripted-response mode (replays a `vector<Message>`, repeating the last entry
  once exhausted) to make agent behavior deterministically testable offline.
- `agents::AgentExecutor` — runs the LLM ↔ tool loop up to `AgentConfig::max_steps`.
- `examples/agent_demo.cpp`.

## [0.2.0] — Tools and structured output

- `parsers::JsonOutputParser` and `parsers::StructuredOutputParser<T>` — parse a `Message`'s content
  as JSON, or straight into a compile-time C++ type via nlohmann::json's `to_json`/`from_json`.
- `tools::Tool`, `tools::FunctionTool`, `tools::ToolRegistry` — define a callable, wrap a plain
  function as one, and render OpenAI- and Anthropic-style function-calling JSON schemas.
- `examples/structured_output.cpp`, `examples/tools_demo.cpp`.

## [0.1.0] — Core + one provider

- `core::Runnable<Input, Output>` and `operator|` composition (`RunnableSequence`,
  `RunnableLambda`), mirroring LangChain's LCEL.
- `core::Message`/`MessageRole`, `core::Document`, `core::Result<T>`/`Error`.
- `llm::ChatModel` base interface, with `MockChat`, `OpenAIChat`, and `AnthropicChat`.
- `prompts::PromptTemplate` / `prompts::ChatPromptTemplate`, `parsers::StrOutputParser`.
- CMake + vcpkg build, initial test suite, `examples/mock_chain.cpp` and `examples/basic_chat.cpp`.
