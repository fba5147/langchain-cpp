# Benchmark results

Comparing `langchain-cpp` against Python LangChain (`langchain-core`) and TypeScript LangChain
(`@langchain/core`) on two axes:

- **Cold start** — process launch to "one chain is built and ready to invoke." A real, well-known
  pain point for Python LangChain (multi-hundred-millisecond import cost); a fair three-way
  comparison since it's dominated by language/runtime + framework overhead, not by anything
  LLM-specific.
- **Mocked chain throughput** — `N` invocations of `prompt | fakeModel | parser` with no network
  calls, measuring ops/sec. Isolates templating/message-construction/dispatch overhead from LLM/API
  latency.

**What's deliberately *not* benchmarked:** end-to-end latency of a real LLM call. That number is
dominated by network round-trip and model inference time, which are identical regardless of which
language's SDK issued the HTTP request — comparing it would tell you about your model/network, not
about the frameworks.

## How to reproduce

```bash
# C++
cmake --build build --target bench_cpp

# Python (needs langchain-core)
python3 -m pip install langchain-core

# TypeScript (needs @langchain/core)
(cd benchmarks/typescript && npm install)

# Run everything
./benchmarks/run_all.sh [iterations] [startup_trials]   # defaults: 100000, 7
```

Each language's benchmark script lives next to this file (`cpp/bench.cpp`, `python/bench.py`,
`typescript/bench.mjs`) and does the equivalent work: build one `ChatPromptTemplate | FakeChatModel |
StrOutputParser` chain, then either exit immediately (`startup` mode) or invoke it `N` times in a
tight loop (`throughput` mode).

## One recorded run

**Environment:** macOS 26.5.1, Apple M3 Max (arm64). Apple clang 21.0.0, CMake 3.28.4,
nlohmann-json 3.12.0, cpr 1.14.2 (Homebrew, x86_64 build — see caveat below). Python 3.11.6,
langchain-core 1.5.3. Node v20.19.3, @langchain/core 1.2.5. `iterations=100000`,
`startup_trials=9`, one discarded warmup run before the C++ startup trials.

| | Cold start (median) | Throughput (ops/sec, mocked chain) |
|---|---|---|
| **C++** (this project) | 0.023 s | ~345,000 |
| **TypeScript** (`@langchain/core`) | 0.148 s | ~63,500 |
| **Python** (`langchain-core`) | 0.521 s | ~3,730 |

Directionally: C++ cold-starts roughly **6x faster than TypeScript** and **23x faster than Python**,
and pushes **~5.4x the throughput of TypeScript** and **~92x the throughput of Python** on a chain
that does no real work beyond string templating and object construction. None of that is surprising —
a compiled binary with no interpreter/VM and no runtime schema validation on every message should win
this comparison — but it's now a measured number instead of an assumption.

The Python gap is consistent with what LangChain's own maintainers have written about: `langchain-core`
builds every message/prompt value through Pydantic model construction and validation, which costs
real time per call that a plain C++ struct doesn't pay.

## Caveats — read before citing these numbers

- **The C++ binary in this run is not native.** This machine is Apple Silicon (arm64), but the only
  `nlohmann-json`/`cpr`/`gtest` available were from an x86_64 Homebrew prefix (`/usr/local`), so
  `bench_cpp` was compiled `-arch x86_64` and ran under Rosetta 2 translation. Python and Node both
  ran as native arm64 binaries. **This means the run above understates C++'s advantage**, not the
  reverse — a native arm64 C++ build (via vcpkg, or an arm64 Homebrew prefix) would be expected to
  start faster and/or push more throughput still.
- **Single machine, single sitting.** These are one set of runs on one laptop, not averaged across
  hardware or repeated over days. Treat the numbers as *directionally* informative, not as a
  precise, reproducible-to-the-percent benchmark suite.
- **The workload is intentionally trivial** (string templating + object construction, no real
  tokenization/parsing/network). It's representative of *framework overhead*, not of what dominates
  a real LLM application's wall-clock time (which is almost always the model call itself).
- **`FakeListChatModel`/`MockChat` are each language's idiomatic "no-network" stand-in**, not
  necessarily doing identical amounts of internal work — Pydantic model construction in
  `langchain-core`'s fake model is itself part of what's being measured, which is fair (it's real
  overhead a Python LangChain app pays), but worth naming explicitly.
