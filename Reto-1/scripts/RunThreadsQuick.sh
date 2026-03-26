#!/bin/bash

# Quick threads-only run with 1 iteration (instead of 10)
# Processes: 2, 4, 8, 16

ROOT_DIR="$(cd "$(dirname "$0")/.." && pwd)"

DEVICE_NAME=$(hostname)
STATS_BASE="$ROOT_DIR/stats/$DEVICE_NAME"
TIMESTAMP=$(date +%Y%m%d_%H%M%S)
STATS_DIR="$STATS_BASE/${TIMESTAMP}_threads_quick"

mkdir -p "$ROOT_DIR/output"
mkdir -p "$STATS_DIR"

echo "[NEW] Corrida rápida de threads: $STATS_DIR"

k=(1 2 3 4 5 6 7 8 9 10 11)
num_proccesses=(2 4 8 16)

THREAD_FILE="$STATS_DIR/threads"
CHECKPOINT_FILE="$STATS_DIR/checkpoint.log"

already_done() {
  local key="$1"
  grep -qF "$key" "$CHECKPOINT_FILE" 2>/dev/null
}

mark_done() {
  local key="$1"
  echo "$key" >>"$CHECKPOINT_FILE"
}

run_safe() {
  local key="$1"
  local output_file="$2"
  shift 2
  local cmd=("$@")

  if already_done "$key"; then
    echo "  [SKIP] $key (ya completado)"
    return
  fi

  "${cmd[@]}" >>"$output_file"
  local exit_code=$?

  if [ $exit_code -eq 0 ]; then
    mark_done "$key"
  else
    echo "  [WARN] $key fallo con codigo $exit_code, continuando..."
  fi
}

# ─── THREADS (1 iteración) ────────────────────────────────────────────────────
echo "Threads testing (1 iteración) ..."

COUNT=0
for n in "${num_proccesses[@]}"; do
  echo "Threads para n=$n ..."
  for i in "${k[@]}"; do
    key="threads,${i},n${n},run1"
    run_safe "$key" "${THREAD_FILE}${num_proccesses[$COUNT]}.csv" "$ROOT_DIR/output/threads" "$i" "$n"
  done
  echo "" >>"${THREAD_FILE}${num_proccesses[$COUNT]}.csv"
  ((COUNT++))
done

echo "Listo! Resultados en: $STATS_DIR"
