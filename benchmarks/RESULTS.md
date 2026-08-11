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
nlohmann-json 3.12.0, cpr 1.14.2, faiss 1.15.0, poppler 25.05.0 (Homebrew, x86_64 build — see caveat
below). Python 3.11.6, langchain-core 1.5.3. Node v20.19.3, @langchain/core 1.2.5. `iterations=100000`,
`startup_trials=9`, one discarded warmup run before the C++ startup trials.

| | Cold start (median) | Throughput (ops/sec, mocked chain) |
|---|---|---|
| **C++** (this project) | 0.066 s | ~338,000 |
| **TypeScript** (`@langchain/core`) | 0.147 s | ~64,000 |
| **Python** (`langchain-core`) | 0.508 s | ~3,740 |

Directionally: C++ cold-starts roughly **2.2x faster than TypeScript** and **7.7x faster than Python**,
and pushes **~5.3x the throughput of TypeScript** and **~90x the throughput of Python** on a chain
that does no real work beyond string templating and object construction. Throughput is essentially
unchanged from the previous recorded run below; cold start is not, and that's worth explaining rather
than glossing over.

### A real regression, and why it's not (yet) fixed

The previous recorded run here measured **0.023 s** for C++ cold start — roughly **3x faster** than
what's measured now. That's a real, reproducible regression (confirmed stable across three separate
runs while updating this file), not noise, and it has a concrete, verified cause:

```
$ otool -L build/benchmarks/cpp/bench_cpp
	/usr/local/opt/cpr/lib/libcpr.1.dylib
	/usr/lib/libcurl.4.dylib
	/usr/local/opt/faiss/lib/libfaiss.dylib
	/usr/local/opt/poppler/lib/libpoppler-cpp.2.dylib
	/usr/lib/libc++.1.dylib
	/usr/lib/libSystem.B.dylib
```

`bench_cpp` now links `libfaiss.dylib` and `libpoppler-cpp.dylib` directly, even though its own code
(`bench.cpp`) never touches `FaissVectorStore` or `PdfLoader` — both added since the last recorded run.
Every process using this library now pays FAISS's/poppler-cpp's dynamic-linker resolution and page-in
cost at startup, whether it uses either feature or not.

This isn't a `target_link_libraries(... PRIVATE ...)` oversight — it was tried, and doesn't help: for a
**static** library (which `langchain_core` is), CMake propagates all link libraries to consumers
regardless of the `PUBLIC`/`PRIVATE`/`INTERFACE` keyword, because the static archive's own object files
(`faiss_vector_store.cpp.o`, `pdf_loader.cpp.o`) still reference `faiss::`/`poppler::` symbols, and
nothing strips them from the archive just because a given consumer never calls them. `PRIVATE` still
keeps FAISS's/poppler-cpp's *include directories* off unrelated consumers' compile lines (real, if
minor, hygiene), but not their link libraries — see the comment in the top-level `CMakeLists.txt`.
Actually fixing this means splitting `FaissVectorStore`/`PdfLoader` into their own library target that
examples/tests opt into, rather than folding everything into one `langchain_core` archive — a real
restructuring, tracked as a follow-up (see CONTRIBUTING.md) rather than done as a drive-by fix here.

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
