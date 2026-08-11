// Benchmark harness mirrored across TypeScript (this file, using
// @langchain/core), C++ (benchmarks/cpp/bench.cpp), and Python
// (benchmarks/python/bench.py, using langchain-core). See
// benchmarks/RESULTS.md for methodology and numbers.
//
// Modes (argv[2], since argv[0]/argv[1] are `node`/the script path):
//   startup    -- build one chain, then exit. The *outer* process wall
//                 time (measured by the calling shell, not this script)
//                 is what's being compared: runtime startup plus
//                 require/import cost plus constructing one chain.
//   throughput -- build one chain, then invoke it argv[3] times with no
//                 network calls (FakeListChatModel), and report ops/sec.

import { ChatPromptTemplate } from "@langchain/core/prompts";
import { StringOutputParser } from "@langchain/core/output_parsers";
import { FakeListChatModel } from "@langchain/core/utils/testing";

async function main() {
  const mode = process.argv[2] ?? "throughput";
  const iterations = process.argv[3] ? parseInt(process.argv[3], 10) : 100_000;

  const prompt = ChatPromptTemplate.fromTemplate("Say hello to {name}.");
  const model = new FakeListChatModel({ responses: ["Hello!"] });
  const parser = new StringOutputParser();
  const chain = prompt.pipe(model).pipe(parser);

  if (mode === "startup") {
    return;
  }

  let totalLen = 0;
  const start = process.hrtime.bigint();
  for (let i = 0; i < iterations; i++) {
    const result = await chain.invoke({ name: "World" });
    totalLen += result.length;
  }
  const end = process.hrtime.bigint();
  const seconds = Number(end - start) / 1e9;

  console.log(
    `language=typescript iterations=${iterations} seconds=${seconds.toFixed(6)} ` +
      `ops_per_sec=${(iterations / seconds).toFixed(1)} checksum=${totalLen}`
  );
}

main();
