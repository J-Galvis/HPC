# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Repository Structure

This repo contains two HPC case studies, each comparing sequential, memory-optimized, threaded, and multiprocessing C implementations of a computational problem:

- **`caso-estudio-1/`** — Matrix multiplication benchmark (see its own `CLAUDE.md` for details)
- **`Reto-1/`** — Jacobi iterative solver for a 1D boundary value problem

## Build and Run

Both projects follow the same pattern:

```bash
cd caso-estudio-1   # or Reto-1
make                # Compiles all variants into output/
make clean          # Removes output/
```

**Caso-estudio-1** (matrix size × matrix size, or matrix size × num_workers):
```bash
./output/secuential 1000 1000
./output/threads 1000 4
./output/multiprocessing 1000 8
```

**Reto-1** (grid parameter `k`, where N = 2^k + 1 interior nodes):
```bash
./output/secuential 8
./output/threads 8 4      # k=8, 4 threads
./output/multiprocessing 8
```

All binaries print a single float (CPU time in seconds, 6 decimal places) to stdout.

## Benchmark Automation

Both projects have `scripts/RunAll.sh` which runs all variants across a range of input sizes, saves CSV results to `stats/<hostname>/<timestamp>/`, and supports checkpoint-based resume on interruption.

```bash
chmod +x scripts/RunAll.sh
./scripts/RunAll.sh
```

## Architecture

### Common Pattern

Every implementation allocates data, fills it with random values, solves the problem, measures CPU time via `getrusage(RUSAGE_SELF)` (plus `RUSAGE_CHILDREN` in multiprocessing variants), prints the result, and frees memory.

### Parallelization Strategies (same across both projects)

| Binary | Strategy |
|--------|----------|
| `secuential` | Naive, compiled without `-O2` |
| `secuentialFlags` | Same source, compiled with `-O2` |
| `memory` | Cache-optimized access pattern |
| `threads` | POSIX threads (`-pthread`), work divided by rows/segments |
| `multiprocessing` | `fork()` + `mmap(MAP_SHARED\|MAP_ANONYMOUS)` for shared result; A/B copied via copy-on-write |

### Reto-1 Specific Details

- **Problem**: Solves u''(x) = -x(x+3)eˣ with u(0)=0, u(1)=0 using Jacobi iteration
- **Convergence**: Iterates until RMS residual < tolerance (~10⁻⁵), checking every 50 iterations in the memory-optimized variant
- **`JacobiMemory.c`** applies four optimizations: fused sweep+residual loop, pointer swapping instead of array copy, infrequent residual checks, reduced memory traffic
- **`JacobiThreads.c`** uses `pthread_barrier_t` for synchronization between iterations — threading hurts performance here due to barrier overhead and too little work per thread for small 1D grids (documented in `docs/Threads.md`)
- **`JacobiMultiprocessing.c`** uses shared memory only for the result vector; boundary exchange between processes happens via the shared array

### Compiler Flags

- Casos-estudio-1: threads binary uses `-pthread`; no `-lm` needed
- Reto-1: all variants use `-Wall -lm`; `secuential` uses `-O0`, `secuentialFlags` uses `-O2`, others use `-O0`
