#!/usr/bin/env python3
"""Benchmark harness mirrored across Python (this file, using
langchain-core), C++ (benchmarks/cpp/bench.cpp), and TypeScript
(benchmarks/typescript/bench.mjs, using @langchain/core). See
benchmarks/RESULTS.md for methodology and numbers.

Modes (argv[1]):
  startup    -- build one chain, then exit. The *outer* process wall time
                (measured by the calling shell, not this script) is what's
                being compared: interpreter startup plus import cost plus
                constructing one chain.
  throughput -- build one chain, then invoke it argv[2] times with no
                network calls (FakeListChatModel), and report ops/sec.
"""

import sys
import time

from langchain_core.language_models.fake_chat_models import FakeListChatModel
from langchain_core.output_parsers import StrOutputParser
from langchain_core.prompts import ChatPromptTemplate


def main() -> None:
    mode = sys.argv[1] if len(sys.argv) > 1 else "throughput"
    iterations = int(sys.argv[2]) if len(sys.argv) > 2 else 100_000

    prompt = ChatPromptTemplate.from_template("Say hello to {name}.")
    model = FakeListChatModel(responses=["Hello!"])
    parser = StrOutputParser()
    chain = prompt | model | parser

    if mode == "startup":
        return

    total_len = 0
    start = time.perf_counter()
    for _ in range(iterations):
        result = chain.invoke({"name": "World"})
        total_len += len(result)
    elapsed = time.perf_counter() - start

    print(
        f"language=python iterations={iterations} seconds={elapsed:.6f} "
        f"ops_per_sec={iterations / elapsed:.1f} checksum={total_len}"
    )


if __name__ == "__main__":
    main()
