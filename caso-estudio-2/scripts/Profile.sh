#!/bin/bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$(dirname "$SCRIPT_DIR")"
cd "$PROJECT_DIR"

TIMESTAMP=$(date +%Y%m%d_%H%M%S)
OUT_DIR="stats/${HOSTNAME}/profiling_${TIMESTAMP}"
mkdir -p "$OUT_DIR"

N=1000

echo "=== Profiling run: N=${N}, host=${HOSTNAME} ==="
echo "Output: $OUT_DIR"

# ── Build ────────────────────────────────────────────────────────────────────
echo ""
echo "[1/3] Building profiling binary (gprof instrumented)..."
make output/profiling 2>&1 | tail -3

# ── CPU profiling with gprof ─────────────────────────────────────────────────
echo ""
echo "[2/3] CPU profiling with gprof (N=${N})..."

rm -f gmon.out
./output/profiling "$N"

gprof -b output/profiling gmon.out > "$OUT_DIR/gprof_flat.txt"
gprof -b -q output/profiling gmon.out > "$OUT_DIR/gprof_callgraph.txt"
cp gmon.out "$OUT_DIR/gmon.out"

echo "  -> gprof_flat.txt"
echo "  -> gprof_callgraph.txt"

# Print top of flat profile for quick inspection
echo ""
echo "--- gprof flat profile (top 10 lines) ---"
head -25 "$OUT_DIR/gprof_flat.txt" | grep -v '^$' | head -15
echo "-----------------------------------------"

# ── Memory profiling ──────────────────────────────────────────────────────────
echo ""
echo "[3/3] Memory profiling (N=${N})..."

if [[ "$(uname)" == "Darwin" ]]; then
    # macOS: /usr/bin/time -l reports peak RSS (resident set size) and page faults
    /usr/bin/time -l ./output/secuential "$N" 2> "$OUT_DIR/memory_peak.txt" || true
    echo "  -> memory_peak.txt (peak RSS from /usr/bin/time -l)"
    echo ""
    echo "--- Memory summary ---"
    grep -E "maximum resident|page reclaims|page faults" "$OUT_DIR/memory_peak.txt" || \
        cat "$OUT_DIR/memory_peak.txt"
    echo "----------------------"
else
    # Linux: valgrind --tool=massif for heap allocation tracking
    if command -v valgrind &>/dev/null; then
        valgrind --tool=massif --massif-out-file="$OUT_DIR/massif.out" \
            ./output/secuential "$N" 2> "$OUT_DIR/valgrind_stderr.txt" || true
        ms_print "$OUT_DIR/massif.out" > "$OUT_DIR/massif_report.txt"
        echo "  -> massif.out + massif_report.txt"
        echo ""
        echo "--- Massif report (top 30 lines) ---"
        head -30 "$OUT_DIR/massif_report.txt"
        echo "-------------------------------------"

        # Also capture peak RSS via /usr/bin/time
        /usr/bin/time -v ./output/secuential "$N" 2> "$OUT_DIR/memory_peak.txt" || true
        echo "  -> memory_peak.txt (peak RSS from /usr/bin/time -v)"
        grep "Maximum resident" "$OUT_DIR/memory_peak.txt" || true
    else
        echo "  valgrind not found — falling back to /usr/bin/time -v"
        /usr/bin/time -v ./output/secuential "$N" 2> "$OUT_DIR/memory_peak.txt" || true
        echo "  -> memory_peak.txt"
        grep "Maximum resident" "$OUT_DIR/memory_peak.txt" || true
    fi
fi

echo ""
echo "=== Done. Results in: $OUT_DIR ==="
ls -lh "$OUT_DIR"
