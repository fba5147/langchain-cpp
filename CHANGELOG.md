# Changelog

All notable changes to this project are documented here. Format loosely follows
[Keep a Changelog](https://keepachangelog.com/en/1.1.0/); this project does not yet follow strict
semantic versioning (pre-1.0, breaking changes can land in a minor bump).

## [0.21.0] — MCP Streamable HTTP transport

Closes the last open item under Part 9 of the roadmap-item-6 gap list: `McpClient`/`McpServer` previously
only spoke stdio.

- `mcp::McpClient(McpHttpConfig{url})` — a second constructor connecting to a remote MCP server over
  "Streamable HTTP" (the transport that replaced the older separate HTTP+SSE transport in MCP's
  2025-03-26 spec revision) instead of spawning a local subprocess. Sends each JSON-RPC message as its
  own HTTP POST with `Accept: application/json, text/event-stream` and an `MCP-Protocol-Version` header;
  handles both response shapes the spec allows (`Content-Type: application/json`, or an SSE-framed
  `text/event-stream` body — real servers, including the official reference server, use the latter even
  for plain request/response exchanges); captures the `Mcp-Session-Id` header from the initialize
  response and echoes it back on every later request. Internally, `McpClient` now dispatches through a
  `detail::ClientTransport` interface with two implementations — `StdioClientTransport` (the previous
  logic, extracted unchanged) and the new `HttpClientTransport` — rather than being hardcoded to stdio.
- `mcp::McpHttpServer` — the server-side mirror: serves a `ToolRegistry` over the same transport, binding
  to `127.0.0.1` by default (per the spec's DNS-rebinding guidance) and rejecting requests whose `Origin`
  header names anything other than a `localhost`/`127.0.0.1` origin. Always answers with a single
  `application/json` response rather than opening an SSE stream — fully spec-compliant (the spec allows
  either), and simpler, since this server never needs to push more than one message per request.
  `McpServer`'s request dispatch was extracted into `handle_message()`, now shared by both the stdio
  `serve()` loop and `McpHttpServer`, so the two transports can't drift in how they interpret
  initialize/tools/list/tools/call.
- Verified against a *real* server on both ends: `McpClient`'s new HTTP transport against `npx
  @modelcontextprotocol/server-everything streamableHttp` (the same reference server used for stdio, run
  in its HTTP mode) — confirming the exact request/response shapes via direct `curl` probing before
  writing the client, not just the spec text — see `examples/mcp_http_client_demo.cpp`; `McpHttpServer`
  against langchain-cpp's own `McpClient` connecting over a real loopback socket, in
  `tests/test_mcp_http_server_roundtrip.cpp` (mirrors `test_mcp_server_roundtrip.cpp`'s stdio round trip,
  but over actual HTTP rather than a subprocess).
- Not implemented, both spec-optional (`MAY`): session-ID *issuance* on `McpHttpServer` (it never assigns
  one) and SSE resumability/redelivery (`Last-Event-ID`) on either side — neither was needed for this
  project's own client/server pair or the reference server it was verified against.
- 16 new tests (7 pure SSE-event-data-extraction, 4 `McpHttpClientLiveContract` against a mock server, 5
  `McpHttpServerRoundtrip` against a real in-process `McpHttpServer`), plus `tests/support/
  mock_http_server.hpp` gained an `extra_headers` field on `MockHttpResponse` (needed to simulate
  `Mcp-Session-Id` response headers in the client-side mock tests) that other providers' contract tests
  can reuse too.

## [0.20.0] — Azure OpenAI and Gemini embeddings

Closes out Part 10 of the roadmap-item-6 gap list (integration breadth): the last open item was more
embeddings providers.

- `rag::AzureOpenAIEmbeddings` — talks to an Azure OpenAI embeddings deployment. Request/response body
  shape is identical to `OpenAIEmbeddings`'s, so both now share `src/rag/embeddings/
  openai_embeddings_wire_format.hpp/.cpp` (newly extracted from `OpenAIEmbeddings`'s previously-inline
  body-building/response-parsing logic, kept pure and separately unit tested); only the deployment-based
  URL (`build_azure_embeddings_url`, alongside the existing `build_azure_chat_completions_url` in
  `azure_url.hpp`) and the `api-key` header differ, same relationship as `AzureOpenAIChat`/`OpenAIChat`.
- `rag::GeminiEmbeddings` — talks to Google's Gemini embeddings API (`embedContent` for `embed_query`,
  `batchEmbedContents` for `embed_documents`). Verified against Google's current docs while writing this
  (not just training-data recall): `taskType`/`outputDimensionality` now nest under an
  `embedContentConfig` object — the older top-level placement is deprecated — and each
  `batchEmbedContents` request entry must repeat the model as `"models/{model}"` even though the URL
  already names it. `embed_query` uses `taskType: RETRIEVAL_QUERY` and `embed_documents` uses
  `RETRIEVAL_DOCUMENT` — a real instance of the query/document embedding distinction the `Embeddings`
  base interface was designed to support, not just theoretical. Defaults to `gemini-embedding-001`
  rather than the newer `gemini-embedding-2`, since the latter dropped `taskType` support in favor of
  folding task instructions into the prompt text — not a drop-in replacement here.
- Both new providers verified via the existing local mock-HTTP-server contract-test harness (`tests/
  support/mock_http_server.hpp`), same approach used for `AnthropicChat`/`GeminiChat`, since neither
  `AZURE_OPENAI_API_KEY` nor `GOOGLE_API_KEY` is available in this environment: proves each client
  builds the documented URL/headers/body and parses the documented response shape correctly, not that
  the real endpoint behaves as documented.
- `examples/more_providers_demo.cpp` gains two more sections (skipping cleanly when credentials aren't
  configured, like the rest of the file).
- 15 new tests (1 Azure embeddings URL, 3 shared OpenAI-embeddings wire format, 4 Gemini-embeddings wire
  format, 4 Azure OpenAI Embeddings live-contract, 3 Gemini Embeddings live-contract). `OpenAIEmbeddings`
  itself had no dedicated tests before this — it's now covered indirectly through the wire-format tests,
  since its body-building/response-parsing logic is exactly what those exercise.

## [0.19.0] — pgvector, CSV/web loaders

Completes Part 10 of the roadmap-item-6 gap list (integration breadth), aside from more embeddings
providers.

- `rag::PgVectorStore` — a `VectorStore` backed by a real Postgres database with the pgvector extension,
  over libpq (found via CMake's own `FindPostgreSQL` module). Same real-persistence contract as
  `QdrantVectorStore`: indexed with one instance, destroyed it, searched successfully from a second
  instance pointed at the same table name, against an actual local `pgvector/pgvector:pg16` container.
  Requires the `vector` extension to already exist in the target database — this store doesn't create it
  itself, since that typically needs superuser privileges its own connection may not have. Uses pgvector's
  cosine distance operator (`<=>`); no ivfflat/hnsw index (exact search only, same tradeoff as
  `InMemoryVectorStore`). Point IDs are handled by Postgres itself (`gen_random_uuid()`), unlike
  `QdrantVectorStore`, which had to generate its own UUIDs client-side.
  `src/rag/vectorstores/pgvector_sql.hpp/.cpp` keeps the SQL string construction pure and separately
  tested, same reasoning as the wire-format modules elsewhere.
- `rag::CsvLoader` — one `Document` per data row (first row is the header); other columns become
  metadata, and content is either every column joined as `"column: value"` lines, or a single named
  column (`Config::content_column`) on its own. Backed by a hand-written RFC-4180-ish CSV parser
  (`src/rag/loaders/csv_parser.hpp/.cpp`, kept pure and separately tested) that correctly handles quoted
  fields with embedded delimiters, embedded newlines, and escaped (`""`) quotes.
- `rag::WebLoader` — fetches a URL (via `cpr`, already a dependency) and strips HTML tags down to plain
  text; `<script>`/`<style>` element contents are dropped entirely, not just their tags. A minimal
  tag-stripping conversion (`src/rag/loaders/html_to_text.hpp/.cpp`, pure and separately tested), not a
  real HTML parser — good enough for a typical article/doc page. Caught and fixed a real bug while
  testing: opening block tags (`<p>`) were inserting a stray space right after the previous element's
  closing-tag newline (e.g. `<p>First</p><p>Second</p>` came out as `"First\n Second"` instead of
  `"First\nSecond"`) — fixed by having opening block tags contribute nothing, since whatever preceded them
  already provided the necessary break.
- `examples/pgvector_demo.cpp` mirrors `examples/qdrant_demo.cpp`: run it twice, the second run detects
  the table already has data (via a probe search) and skips re-indexing.
- `tests/test_pgvector_store_live.cpp` follows `test_qdrant_vector_store_live.cpp`'s pattern:
  `GTEST_SKIP()`s when no Postgres server is reachable, so `ctest` stays green without Docker running.
- 34 new tests (7 pgvector SQL, 5 pgvector live, 8 CSV parser, 5 CsvLoader, 7 html_to_text, 2 WebLoader —
  covering pure logic and, for both new vector stores, real round trips against actual servers).

## [0.18.0] — Qdrant vector store

Part 10 of the roadmap-item-6 gap list (further integration breadth, alongside `FaissVectorStore`/
`PdfLoader` from v0.15.0).

- `rag::QdrantVectorStore` — a `VectorStore` backed by a real Qdrant server over its REST API. Unlike
  `InMemoryVectorStore`/`FaissVectorStore` (both in-process), data lives on a separate process and
  survives this one exiting, and can be shared across processes or machines — verified for real:
  indexed documents with one `QdrantVectorStore` instance, destroyed it, and searched successfully from
  a second instance pointed at the same collection name (simulating a process restart), against an
  actual local Qdrant server (`docker run -p 6333:6333 qdrant/qdrant`).
- The collection is created lazily on first `add_documents()` (once the embedding dimension is known)
  or reused as-is if one with the configured name already exists. Qdrant's "Cosine" distance metric
  normalizes vectors internally, so no client-side normalization is needed (unlike `FaissVectorStore`'s
  `IndexFlatIP`, which does need it). Point IDs are randomly generated UUIDv4s, since Qdrant requires
  point IDs to be either an unsigned integer or a UUID — a plain incrementing counter or arbitrary
  string won't do, confirmed by testing against the real API and hitting its validation error directly.
- `src/rag/vectorstores/qdrant_wire_format.hpp/.cpp` — the request/response conversion logic, kept pure
  and separately unit-tested, same reasoning as the provider wire-format modules.
- `examples/qdrant_demo.cpp` — the same load/split/index/retrieve/answer pipeline as `rag_demo.cpp`, but
  against a real Qdrant server; run twice, the second run detects the collection already has data (via
  a probe search) and skips re-indexing rather than inserting duplicate points, since
  `QdrantVectorStore` itself doesn't dedupe on `add_documents()`.
- `tests/test_qdrant_vector_store_live.cpp` — live tests against a real Qdrant server, `GTEST_SKIP()`ing
  (not failing) when none is reachable, so `ctest` stays green without Docker running while still
  exercising the real HTTP round trip when it is. All passed against a real local instance while
  building this, including the cross-instance persistence test above.
- 12 new tests (7 pure wire-format, 5 live).

## [0.17.0] — MCP server

Completes Part 9 of the roadmap-item-6 gap list: `mcp::McpClient` (v0.14.0) covered connecting *to* an
MCP server; this adds the other half, exposing langchain-cpp's own tools *as* one.

- `mcp::McpServer` — wraps a `tools::ToolRegistry` and serves it over stdio: reads newline-delimited
  JSON-RPC requests from an `std::istream` (real stdin by default), dispatches
  `initialize`/`tools/list`/`tools/call`, and writes responses to an `std::ostream` (real stdout by
  default). Symmetric with `McpClient` — only the tools capability, no resources/prompts/sampling.
- New JSON-RPC framing on the server side (`jsonrpc.hpp`): `is_request`/`parse_request` (a request has
  both `method` and `id`; a notification has `method` but no `id`), `build_success_response`,
  `build_error_response`.
- New MCP-specific server-side shapes (`mcp_protocol.hpp`): `build_initialize_result`,
  `build_tools_list_result`, `build_call_tool_result` (encodes a `Tool`'s `Result<json>` — a string
  value becomes the text verbatim, anything else is JSON-dumped into it, an error becomes
  `isError: true` — the exact mirror of how `McpClient::call_tool` decodes a response).
- `examples/mcp_server_demo.cpp` — a real MCP server exposing a `calculator` tool, meant to be spawned
  by an MCP client (an external one, or langchain-cpp's own).
- `tests/test_mcp_server_roundtrip.cpp` — rather than only unit-testing the server's pure functions, this
  spawns the *built* `mcp_server_demo` binary via `McpClient` and does a real round trip: `list_tools`
  discovers the calculator, `call_tool` computes a real result, calling an unknown tool throws, and
  `mcp::as_tools()` wraps it for an agent — this project's own client and server actually interoperating,
  verified automatically and offline (no npx/network needed), unlike `examples/mcp_client_demo.cpp`'s
  live run against the external reference server.
- 14 new tests (10 pure-function, 4 round-trip).

## [0.16.0] — AnthropicChat/GeminiChat HTTP-layer contract tests

Addresses a real credibility gap: the README admitted `AnthropicChat`/`GeminiChat` were "written to
spec" but never exercised over real HTTP, with no way to close that without API credentials.

- `tests/support/mock_http_server.hpp/.cpp` — a minimal HTTP/1.1 server (raw POSIX sockets, single
  connection at a time, no TLS) for tests: binds to an OS-assigned port on 127.0.0.1 and runs a handler
  on a background thread, so a real `ChatModel` can be pointed at it via `base_url` and exercised over
  actual sockets rather than an in-process mock.
- While writing these tests, cross-checked both providers' implementations against the current official
  docs (Anthropic Messages API, Google's `generateContent` REST reference) rather than trusting memory —
  this surfaced that Gemini's `x-goog-api-key` header (already what this project uses) is the *current*
  recommended auth method, not the legacy `?key=...` query parameter older guides show. No wire-format
  bugs found; both implementations already matched the documented contract exactly.
- `tests/test_anthropic_chat_live_contract.cpp` / `tests/test_gemini_chat_live_contract.cpp` — 8 tests
  verifying, over a real HTTP round trip: correct URL (`POST /v1/messages`,
  `POST /v1beta/models/{model}:generateContent`), correct headers (`x-api-key`/`anthropic-version`,
  `x-goog-api-key`), correct request bodies (including `system`/`systemInstruction` and bound tools),
  correct parsing of both text and tool-call responses, and correct error surfacing (the server's own
  error body) on a non-200 status. All passed on the first run.
- This is deliberately not overclaimed as "verified live": it proves both clients faithfully implement
  the documented contract end-to-end through real sockets, not that Anthropic's/Google's actual servers
  behave identically to their own docs (auth/rate-limit edge cases and undocumented behavior are still
  unverified — see the README section this adds). Closing that last gap still needs real credentials.

## [0.15.0] — FAISS vector store and PDF loader

Part 10 of the roadmap-item-6 gap list (partial — integration breadth).

- `rag::FaissVectorStore` — a `VectorStore` backed by FAISS's `IndexFlatIP`: exact nearest-neighbor
  search via inner product over L2-normalized vectors (i.e. cosine similarity), rather than
  `InMemoryVectorStore`'s own hand-rolled brute-force loop. `faiss::Index` is only forward-declared in
  the public header, so FAISS's own headers don't leak into every translation unit that includes it.
- `rag::PdfLoader` — loads a PDF as one `Document` per page (mirrors LangChain's `PyPDFLoader`), tagged
  with `metadata["source"]`/`metadata["page"]`. Uses poppler-cpp; also kept out of the public header.
- CMake now locates FAISS (its CMake config target name isn't consistent across
  vcpkg/Homebrew — vcpkg exports `faiss::faiss`, Homebrew's bottle exports a bare `faiss`; both are
  tried, falling back to a manual `find_path`/`find_library` as a last resort) and poppler-cpp (via
  `pkg_check_modules`, the same approach poppler's own vcpkg port documents using). Added `faiss` and
  `poppler` to `vcpkg.json`; the vcpkg path itself wasn't run end-to-end in this environment (that means
  building both from source via vcpkg), unlike the Homebrew path, which was.
- `PdfLoader`'s tests use hand-crafted, byte-exact minimal PDF fixtures (correct xref table and all)
  embedded directly in the test file, rather than shipping binary PDF files or depending on an external
  PDF-generation tool — verified to open correctly with `pdftotext` while building this.
- 9 new tests (5 for `FaissVectorStore`, 4 for `PdfLoader`).

## [0.14.0] — MCP client

Part 9 of the roadmap-item-6 gap list.

- `mcp::McpClient` — talks JSON-RPC 2.0 to an MCP (Model Context Protocol) server over stdio: spawns it
  as a subprocess (POSIX fork/exec/pipe; the project's CI only targets ubuntu-latest/macos-latest, so no
  Windows path), then `initialize()` (handshake), `list_tools()`, and `call_tool()`. Resources, prompts,
  and sampling aren't implemented — out of scope for what `AgentExecutor` needs.
- `mcp::as_tools(client)` — wraps every tool a connected client's server exposes as a
  `langchain::tools::Tool` (a `FunctionTool` that calls back into the client), so they drop straight into
  a `ToolRegistry` and get called by `AgentExecutor` like any other tool. Mirrors LangChain.js's
  `@langchain/mcp-adapters`.
- The generic JSON-RPC framing (`build_request`/`parse_response`/...) and the MCP-specific message
  shapes (`build_initialize_params`/`parse_tools_list_result`/`parse_call_tool_result`) are both pure,
  directly-testable functions, same reasoning as the provider wire-format modules. The stdio transport
  itself is tested against real child processes (`/bin/cat`, `/bin/echo`) rather than a mock, so the
  actual fork/exec/pipe code path is exercised for real.
- `examples/mcp_client_demo.cpp` — verified live end-to-end against the official reference server (`npx
  @modelcontextprotocol/server-everything`): `initialize()`, `list_tools()` (discovering all 13 tools),
  `call_tool()` for both a success case and an error case (a nonexistent tool name, confirming it
  surfaces as a thrown `std::runtime_error`), and then a real `AgentExecutor` loop backed by local Ollama
  (`llama3.2`) correctly choosing the MCP-backed `get-sum` tool and getting the right answer back through
  the full stdio round trip.
- 17 new tests.

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
