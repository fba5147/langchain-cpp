# Contributing to langchain-cpp

Thanks for considering a contribution. This project is an unofficial, idiomatic-C++ take on
LangChain's concepts — see the [README](README.md) for the project's scope and design philosophy
before diving in, especially the note on *not* aiming for line-for-line API parity with the Python
library.

## Building and testing

Requires a C++20 compiler, CMake 3.21+, `pkg-config`, and [vcpkg](https://github.com/microsoft/vcpkg)
(dependencies: `nlohmann-json`, `cpr`, `gtest`, `faiss`, `poppler`).

```bash
export VCPKG_ROOT=/path/to/vcpkg
cmake --preset default
cmake --build build
ctest --test-dir build --output-on-failure
```

If you don't have vcpkg set up, the same dependencies are available via Homebrew
(`brew install nlohmann-json cpr googletest faiss poppler pkg-config`) — configure with
`cmake -S . -B build -G Ninja -DCMAKE_PREFIX_PATH=/usr/local` (or `/opt/homebrew` on Apple Silicon
with the ARM Homebrew prefix) instead of the vcpkg preset. This is the path used for most local
development (see the README's Ollama/local-testing notes), but the vcpkg path (`cmake --preset
default`) has also been verified end-to-end on macOS (Configure + Build + full test suite passing) —
see `triplets/arm64-osx.cmake`/`triplets/x64-osx.cmake` for two real, non-obvious fixes that were
needed to get there: AppleClang doesn't support OpenMP out of the box (needed by faiss), and faiss's
Metal GPU backend defaults on for Apple Silicon and needs a component (Apple's Metal Toolchain) that
isn't installed on CI runners by default, so it's turned off (this project only uses `IndexFlatIP`,
CPU-only).

Run an example or two to sanity-check behavior beyond what the unit tests cover — see the list in the
README's "Building" section. Most examples (`mock_chain`, `structured_output`, `tools_demo`,
`agent_demo`, `rag_demo`, `callbacks_demo`, `caching_demo`, `chat_history_demo`, `few_shot_demo`,
`rate_limit_demo`, `multimodal_demo`) run fully offline against `MockChat`. `ollama_demo` and
`basic_chat`/`more_providers_demo`/`mcp_client_demo` (with an API key, or `ollama serve` running
locally) exercise real network calls. `qdrant_demo` needs a real Qdrant server (`docker run -p 6333:6333
qdrant/qdrant`) but otherwise runs offline (`MockEmbeddings`). `mcp_server_demo` isn't meant to be run
directly at all — it speaks stdio JSON-RPC, not a human-typed REPL; it's spawned by an MCP client (see
`tests/test_mcp_server_roundtrip.cpp`).

`tests/test_qdrant_vector_store_live.cpp` follows the same pattern as the mock-server contract tests
below, but against a *real* dependency instead of a mock: it `GTEST_SKIP()`s (not fails) when no Qdrant
server is reachable at `QDRANT_URL` (default `http://localhost:6333`), so `ctest` stays green without
Docker running, but exercises the real HTTP round trip end-to-end when it is.

## Before opening a PR

- Add or update tests for behavior you change — every module in this codebase has unit tests, and new
  code should too.
- Run the full test suite (`ctest --test-dir build`) and confirm it's green.
- If you touch a provider's wire-format code, sanity check it beyond `MockChat`-only unit tests, which
  won't catch real wire-format bugs. For `OpenAIChat`/`AzureOpenAIChat`/`OpenAIEmbeddings`, an
  OpenAI-compatible local server via Ollama is a free, fast way to do this — see
  `examples/ollama_demo.cpp`. For `AnthropicChat`/`GeminiChat`, where no local server speaks their wire
  format, run (and extend, if your change touches new request/response shapes)
  `tests/test_anthropic_chat_live_contract.cpp`/`tests/test_gemini_chat_live_contract.cpp` — these spin
  up a local mock server (`tests/support/mock_http_server.hpp`) implementing the documented contract and
  exercise the real HTTP layer against it. If you have real Anthropic/Google credentials, testing
  against the actual endpoint too is even more valuable (see the "Where to help" note on this below).
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
prompting/`OutputFixingParser`), 7 (`RateLimiter`/`RateLimitedChatModel`), 8 (`Message::images`), and 9
(`mcp::McpClient` + `mcp::McpServer`) are done. Part 10 (integration breadth) is partly done
(`FaissVectorStore`, `QdrantVectorStore`, `PdfLoader`); concrete things worth doing, roughly in priority
order:

- **More integration breadth** (part 10) — `FaissVectorStore`, `QdrantVectorStore`, and `PdfLoader`
  shipped; still open: pgvector vector store, CSV/web-HTML document loaders, more embeddings providers.
- **Split `langchain_core` into multiple library targets** — adding FAISS/poppler-cpp as dependencies of
  the single monolithic `langchain_core` static library means every consumer (every example, the
  benchmark binary) now links `libfaiss`/`libpoppler-cpp` and pays their dynamic-linker load cost at
  startup, even if it never touches `FaissVectorStore`/`PdfLoader` — confirmed via `otool -L` and
  measured as a real ~3x cold-start regression in `benchmarks/RESULTS.md`. `target_link_libraries(...
  PRIVATE ...)` doesn't fix this for a *static* library (CMake still propagates link libraries to
  consumers regardless of the keyword); actually fixing it means splitting optional-dependency features
  into their own linkable targets that consumers opt into.
- **An HTTP/SSE transport for MCP** — both `McpClient` and `McpServer` (part 9) only speak stdio today
  (the common case for local servers); a remote/HTTP transport is a natural, separable follow-up.
- **A LangSmith-equivalent tracing backend** — part 3 shipped the hook points
  (`CallbackHandler`/`CallbackManager`/`CallbackingChatModel`/`CallbackingTool`) and a console printer,
  but nothing that persists/visualizes traces. A `CallbackHandler` that writes structured JSON lines
  (or exports to an existing tracing system) would build directly on that.
- **Streaming for `AnthropicChat`/`GeminiChat`** — only `OpenAIChat`/`AzureOpenAIChat` got real
  incremental streaming (part 2); the other two still use `ChatModel::stream()`'s default (one
  synthesized final chunk), same caveat as their non-streaming tool-calling below.
- **Live coverage for `AnthropicChat` tool-calling and `GeminiChat` against the real endpoints** — both
  are written to spec (pure request/response conversion has unit tests —
  `tests/test_gemini_wire_format.cpp`/`tests/test_anthropic_wire_format.cpp`) and their full HTTP layer
  (URL, headers, request/response bodies, error handling) is verified end-to-end against a local mock
  server that implements the documented contract (`tests/test_anthropic_chat_live_contract.cpp`,
  `tests/test_gemini_chat_live_contract.cpp` — see the README section on what this does and doesn't
  prove) — but neither has had the equivalent of the `ollama_demo.cpp` live smoke test against the
  *actual* api.anthropic.com/generativelanguage.googleapis.com, since no Anthropic/Google credentials
  were available while building this. If you have one, running `examples/more_providers_demo.cpp`
  (Gemini) or a quick Anthropic tool-calling script against a real key and reporting back what broke is
  high-value — that's specifically the risk the mock can't cover (auth/rate-limit edge cases,
  undocumented server behavior).
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
