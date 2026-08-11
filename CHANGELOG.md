# Changelog

All notable changes to this project are documented here. Format loosely follows
[Keep a Changelog](https://keepachangelog.com/en/1.1.0/); this project does not yet follow strict
semantic versioning (pre-1.0, breaking changes can land in a minor bump).

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
