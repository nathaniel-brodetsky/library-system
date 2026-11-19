#!/usr/bin/env bash
set -euo pipefail

cd "$(dirname "$0")/.."

SERVER=./build/library_server
JMETER=${JMETER:-jmeter}
PORT=${PORT:-8080}
LOOPS=${LOOPS:-1000}
RAMPUP=${RAMPUP:-2}
STEPS=${STEPS:-"10 50 100 200"}
OUT=load/results

if [[ ! -x "$SERVER" ]]; then
    echo "Build the project first: cmake --build build" >&2
    exit 1
fi

if ss -ltn | grep -q ":$PORT "; then
    echo "Port $PORT is already in use, run with PORT=<free port>" >&2
    exit 1
fi

rm -rf "$OUT" load/results.tar.gz
mkdir -p "$OUT"

for threads in $STEPS; do
    copies=$((threads * LOOPS))
    if ((copies > 999999)); then
        echo "threads * loops must not exceed 999999" >&2
        exit 1
    fi

    echo "=== $threads threads x $LOOPS iterations"
    "$SERVER" --port "$PORT" --copies "$copies" >"$OUT/server-$threads.log" 2>&1 &
    pid=$!
    for _ in $(seq 1 100); do
        curl -sf "http://localhost:$PORT/health" >/dev/null && break
        if ! kill -0 "$pid" 2>/dev/null; then
            echo "Server failed to start:" >&2
            cat "$OUT/server-$threads.log" >&2
            exit 1
        fi
        sleep 0.2
    done

    "$JMETER" -n -t load/library_load.jmx \
        -Jthreads="$threads" -Jloops="$LOOPS" -Jrampup="$RAMPUP" -Jport="$PORT" \
        -l "$OUT/results-$threads.jtl" -e -o "$OUT/report-$threads" -j "$OUT/jmeter-$threads.log"

    curl -s "http://localhost:$PORT/stats" >"$OUT/stats-$threads.json"
    ps -o rss= -p "$pid" >"$OUT/memory-kb-$threads.txt"
    kill "$pid"
    wait "$pid" 2>/dev/null || true
done

{
    uname -sr
    lscpu | grep -E 'Model name|^CPU\(s\)'
    free -h | grep Mem
    "$JMETER" --version 2>/dev/null | grep -m1 -oE '[0-9]+\.[0-9]+(\.[0-9]+)?' || true
} >"$OUT/machine.txt"

gzip -f "$OUT"/*.jtl
tar czf load/results.tar.gz -C load results
echo "Done: load/results.tar.gz"
