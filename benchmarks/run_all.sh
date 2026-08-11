#!/usr/bin/env bash
# Runs the cold-start and mocked-chain-throughput benchmarks for all three
# languages and prints a summary. See benchmarks/RESULTS.md for
# methodology, one recorded run, and caveats -- these numbers are
# machine- and run-specific, not a reproducible authoritative claim.
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"
ITERATIONS="${1:-100000}"
STARTUP_TRIALS="${2:-7}"

CPP_BIN="${REPO_ROOT}/build/benchmarks/cpp/bench_cpp"
PY_BIN="python3"
PY_SCRIPT="${SCRIPT_DIR}/python/bench.py"
NODE_BIN="node"
TS_SCRIPT="${SCRIPT_DIR}/typescript/bench.mjs"

if [ ! -x "${CPP_BIN}" ]; then
    echo "C++ benchmark not built. Run: cmake --build build --target bench_cpp" >&2
    exit 1
fi
if [ ! -d "${SCRIPT_DIR}/typescript/node_modules" ]; then
    echo "TypeScript deps not installed. Run: (cd benchmarks/typescript && npm install)" >&2
    exit 1
fi

# Prints the median of N wall-clock times (seconds) for `cmd ... startup`.
median_startup_seconds() {
    local -a times=()
    local i
    for ((i = 0; i < STARTUP_TRIALS; i++)); do
        local start end
        start=$(date +%s.%N)
        "$@" startup >/dev/null
        end=$(date +%s.%N)
        times+=("$(echo "${end} - ${start}" | bc)")
    done
    printf '%s\n' "${times[@]}" | sort -n | awk '{a[NR]=$1} END {print a[int((NR+1)/2)]}'
}

echo "=== Cold-start (median of ${STARTUP_TRIALS} runs, one warmup discarded) ==="

"${CPP_BIN}" startup >/dev/null # warmup: first exec after build pays a one-time dynamic-linker page-in cost
cpp_startup=$(median_startup_seconds "${CPP_BIN}")
echo "cpp:        ${cpp_startup}s"

py_startup=$(median_startup_seconds "${PY_BIN}" "${PY_SCRIPT}")
echo "python:     ${py_startup}s"

(cd "${SCRIPT_DIR}/typescript" && ts_startup=$(median_startup_seconds "${NODE_BIN}" "${TS_SCRIPT}") && echo "typescript: ${ts_startup}s")

echo ""
echo "=== Mocked chain throughput (${ITERATIONS} invocations, no network) ==="
"${CPP_BIN}" throughput "${ITERATIONS}"
"${PY_BIN}" "${PY_SCRIPT}" throughput "${ITERATIONS}"
(cd "${SCRIPT_DIR}/typescript" && "${NODE_BIN}" "${TS_SCRIPT}" throughput "${ITERATIONS}")
