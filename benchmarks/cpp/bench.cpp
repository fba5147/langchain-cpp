// Benchmark harness mirrored across C++ (this file), Python
// (benchmarks/python/bench.py, using langchain-core), and TypeScript
// (benchmarks/typescript/bench.mjs, using @langchain/core). See
// benchmarks/RESULTS.md for methodology and numbers.
//
// Two modes, selected by argv[1]:
//   startup    -- build one chain, then exit. The *outer* process wall
//                 time (measured by the calling shell, not this program)
//                 is what's being compared: interpreter/runtime startup
//                 plus whatever each language's import/require machinery
//                 costs, plus constructing one chain.
//   throughput -- build one chain, then invoke it argv[2] times with no
//                 network calls (MockChat), and report ops/sec. Isolates
//                 framework overhead (templating, message construction,
//                 dispatch) from LLM/API latency.

#include "langchain/langchain.hpp"

#include <chrono>
#include <cstdlib>
#include <iostream>
#include <string>

using namespace langchain;

int main(int argc, char** argv) {
    std::string mode = argc > 1 ? argv[1] : "throughput";
    long iterations = argc > 2 ? std::atol(argv[2]) : 100000;

    auto prompt = std::make_shared<prompts::ChatPromptTemplate>("Say hello to {name}.");
    auto model = std::make_shared<providers::MockChat>("Hello!");
    auto parser = std::make_shared<parsers::StrOutputParser>();
    auto chain = prompt | model | parser;

    if (mode == "startup") {
        return 0;
    }

    std::size_t total_len = 0;
    auto start = std::chrono::steady_clock::now();
    for (long i = 0; i < iterations; ++i) {
        std::string result = chain->invoke({{"name", "World"}});
        total_len += result.size();
    }
    auto end = std::chrono::steady_clock::now();
    double seconds = std::chrono::duration<double>(end - start).count();

    std::cout << "language=cpp iterations=" << iterations << " seconds=" << seconds
              << " ops_per_sec=" << (static_cast<double>(iterations) / seconds) << " checksum=" << total_len
              << '\n';
    return 0;
}
