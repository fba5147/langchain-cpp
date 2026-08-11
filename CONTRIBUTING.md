# Contributing to langchain-cpp

Thanks for considering a contribution. This project is an unofficial, idiomatic-C++ take on
LangChain's concepts — see the [README](README.md) for the project's scope and design philosophy
before diving in, especially the note on *not* aiming for line-for-line API parity with the Python
library.

## Building and testing

Requires a C++20 compiler, CMake 3.21+, and [vcpkg](https://github.com/microsoft/vcpkg) (dependencies:
`nlohmann-json`, `cpr`, `gtest`).

```bash
export VCPKG_ROOT=/path/to/vcpkg
cmake --preset default
cmake --build build
ctest --test-dir build --output-on-failure
```

If you don't have vcpkg set up, the same dependencies are available via Homebrew
(`brew install nlohmann-json cpr googletest`) — configure with
`cmake -S . -B build -G Ninja -DCMAKE_PREFIX_PATH=/usr/local` (or `/opt/homebrew` on Apple Silicon
with the ARM Homebrew prefix) instead of the vcpkg preset.

Run an example or two to sanity-check behavior beyond what the unit tests cover — see the list in the
README's "Building" section. Several examples (`ollama_demo`, `basic_chat` with an API key) exercise
real network calls; `mock_chain`, `structured_output`, `tools_demo`, `agent_demo`, and `rag_demo` run
fully offline.

## Before opening a PR

- Add or update tests for behavior you change — every module in this codebase has unit tests, and new
  code should too.
- Run the full test suite (`ctest --test-dir build`) and confirm it's green.
- If you touch a provider's wire-format code (`OpenAIChat`, `AnthropicChat`, `OpenAIEmbeddings`), sanity
  check it against a real endpoint if you can (an OpenAI-compatible local server via Ollama is a free,
  fast way to do this — see `examples/ollama_demo.cpp`). Wire-format bugs don't show up in unit tests
  that only exercise `MockChat`.
- Keep commits and PRs scoped to one change; separate refactors from behavior changes.

## Code conventions

- C++20, no exceptions to the standard library style already in the codebase: `Runnable<Input,
  Output>` subclasses for anything that transforms data through a chain, `std::shared_ptr` for
  anything held across an `operator|` composition, `core::Result<T>` (not exceptions) for expected
  failure modes callers should handle (e.g. `Tool::call`), exceptions for genuine misuse/programmer
  error (e.g. a provider constructed with no API key).
- No comments explaining *what* code does — name things so the code reads on its own. A comment is for
  a non-obvious *why*: a wire-format quirk, a subtle invariant, a workaround for a specific provider
  bug.
- Match the existing file layout: one class (or a couple of tightly related ones) per header, mirrored
  by a `.cpp` unless it's template-only or trivially small.
- Don't add abstractions ahead of a concrete second use case. If you're building a new provider or
  module and it looks like an existing one, copy the shape rather than inventing a shared base class
  for two implementations.

## Where to help

The [README's roadmap](README.md#roadmap) tracks feature milestones in build order. Beyond that,
concrete things worth doing:

Roadmap item 6 (README) is a lettered list of gaps identified against official LangChain
(Python)/LangChain.js; parts 1 (`RunnableParallel`/`RunnablePassthrough`/`RunnableBranch`), 2
(`ChatModel::stream()`), 3 (callbacks), 4 (`CachingChatModel`), 5 (`ChatModelWithHistory`), 6 (few-shot
prompting/`OutputFixingParser`), 7 (`RateLimiter`/`RateLimitedChatModel`), and 8 (`Message::images`)
are done, so parts 9-10 are the open ones, roughly in priority order:

- **MCP client/server support** (part 9) — confirmed as an official package
  (`@langchain/mcp-adapters`), likely the most differentiated piece relative to other C++ LLM libraries.
- **A LangSmith-equivalent tracing backend** — part 3 shipped the hook points
  (`CallbackHandler`/`CallbackManager`/`CallbackingChatModel`/`CallbackingTool`) and a console printer,
  but nothing that persists/visualizes traces. A `CallbackHandler` that writes structured JSON lines
  (or exports to an existing tracing system) would build directly on that.
- **Streaming for `AnthropicChat`/`GeminiChat`** — only `OpenAIChat`/`AzureOpenAIChat` got real
  incremental streaming (part 2); the other two still use `ChatModel::stream()`'s default (one
  synthesized final chunk), same caveat as their non-streaming tool-calling below.
- **Live coverage for `AnthropicChat` tool-calling and `GeminiChat`** — both are written to spec
  (pure request/response conversion has unit tests for both now — `tests/test_gemini_wire_format.cpp`
  and `tests/test_anthropic_wire_format.cpp`) but neither has had the equivalent of the
  `ollama_demo.cpp` live smoke test that `OpenAIChat` got — no Anthropic/Google credentials were
  available while building this. If you have one, running `examples/more_providers_demo.cpp` (Gemini)
  or a quick Anthropic tool-calling script against a real key and reporting back what broke is
  high-value.
- **A real vector store integration** (FAISS, Qdrant, pgvector, ...) alongside `InMemoryVectorStore`
  (part 10).
- **Image support for `AnthropicChat`/`GeminiChat`** — both throw on `Message::images` today (part 8
  shipped `OpenAIChat`/`AzureOpenAIChat` support only); wiring up their image content-block formats is
  a natural, self-contained follow-up.
- **More `ExampleSelector` implementations** (length-based, max-marginal-relevance) alongside
  `SemanticSimilarityExampleSelector` (part 6).
- **Model-aware rate limiting** (token/cost-based, not just requests-per-second) alongside
  `RateLimiter` (part 7) — needs per-provider tokenization, which is its own scoping question.
- **A persistent `ChatModelCache`** (file/SQL/Redis-backed) alongside `InMemoryChatModelCache` (part 4
  shipped the interface and an in-memory implementation, not persistence across process restarts).
- **A SQL/Redis-backed `ChatMessageHistory`** alongside `InMemoryChatMessageHistory`/
  `FileChatMessageHistory` (part 5) — useful once a conversation needs to be shared across processes
  rather than owned by whichever process has the file open.
- **CI matrix breadth** — the current workflow builds on Linux and macOS; a Windows/MSVC leg would
  catch platform-specific bugs (case-sensitive includes, `<filesystem>` quirks, etc.).

If you're unsure whether an idea fits the project's scope, open an issue to discuss before sending a
large PR.

## Reporting bugs / requesting features

Use the GitHub issue templates. For a bug, include your compiler/OS, the exact CMake configure command,
and — if it's a provider-related bug — whether you can reproduce it against a local Ollama server
(rules out account/API-key issues).
