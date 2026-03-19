# AGENTS.md - HPC Matrix Multiplication Project

This repository benchmarks different approaches to matrix multiplication: sequential, memory-optimized, threaded (pthreads), and multiprocess (fork).

## Build Commands

All source files are in `src/`. Compiled binaries go in `output/`.

### Individual Builds
```bash
gcc src/SecuentialMatrixSolver.c -o output/secuential
gcc src/MemoryMatrixSolver.c -O3 -o output/memory
gcc src/ThreadsMatrixSolver.c -o output/threads -lpthread
gcc src/MultiprocessingMatrixSolver.c -o output/multiprocessing
```

### Build All with Warnings
```bash
for solver in secuential memory threads multiprocessing; do
    case $solver in
        secuential)     gcc -Wall -Wextra src/SecuentialMatrixSolver.c -o output/$solver ;;
        memory)         gcc -Wall -Wextra -O3 src/MemoryMatrixSolver.c -o output/$solver ;;
        threads)        gcc -Wall -Wextra src/ThreadsMatrixSolver.c -o output/$solver -lpthread ;;
        multiprocessing) gcc -Wall -Wextra src/MultiprocessingMatrixSolver.c -o output/$solver ;;
    esac
done
```

### Run a Single Test (3x3 verification)
```bash
# 1. Uncomment test_3x3() call in main()
# 2. Recompile
sed -i 's|// test_3x3();|test_3x3();|' src/SecuentialMatrixSolver.c
gcc src/SecuentialMatrixSolver.c -o output/secuential
./output/secuential 3 3

# 3. Revert after verification
sed -i 's|test_3x3();|// test_3x3();|' src/SecuentialMatrixSolver.c
```

## Run Commands

### Sequential & Memory-Optimized
```bash
./output/secuential <rows> <cols>
./output/memory <rows> <cols>
```

### Threaded & Multiprocessing
```bash
./output/threads <rows> <num_threads>
./output/multiprocessing <rows> <num_processes>
```

### Benchmark Example
```bash
./output/secuential 1000 1000  # Output: 0.123456,
```

Output format: CSV-friendly with elapsed time in seconds (6 decimals) + comma.

## Code Style Guidelines

### Naming Conventions
- **Functions**: PascalCase (`CreateMatrix`, `FreeMatrix`, `MultiplyMatrices`)
- **Variables**: camelCase (`startRow`, `endRow`, `numThreads`)
- **Structs**: PascalCase (`ThreadArgs`, `ProcessArgs`)
- **Globals**: Prefix with `g_` if needed (currently unused)

### Brace Style
K&R style (opening brace on same line):
```c
void MyFunction() {
    if (condition) {
        doSomething();
    }
}
```

### C Standard
- Use C99 standard (`gcc -std=c99`)
- Loop variables use `int` (C99 feature)
- **Avoid VLAs** - use heap allocation (`malloc`) instead

### Includes
Order logically with brief comments:
```c
#include <stdio.h>      // Standard input/output
#include <stdlib.h>     // Memory management, atoi, rand
#include <time.h>       // clock_gettime for wall-clock timing
#include <pthread.h>    // POSIX threads
#include <unistd.h>     // fork, getpid
#include <sys/wait.h>   // wait for child processes
#include <sys/mman.h>   // shared memory mmap
```

### Error Handling
Always check return values:
```c
int** matrix = (int**)malloc(rows * sizeof(int*));
if (!matrix) { perror("malloc failed (matrix rows)"); exit(1); }

for (int i = 0; i < rows; i++) {
    matrix[i] = (int*)malloc(cols * sizeof(int));
    if (!matrix[i]) { perror("malloc failed (matrix col)"); exit(1); }
}

// For mmap, check MAP_FAILED (not NULL):
int* data = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
if (data == MAP_FAILED) { perror("mmap failed"); exit(1); }
```

Use `perror()` for system errors, `fprintf(stderr, ...)` for custom messages.

### Thread/Process Safety
- Cap thread/process count to matrix rows: `if (num > rows) num = rows;`
- Always join/join threads after creation:
```c
for (int t = 0; t < num_threads; t++) {
    pthread_join(threads[t], NULL);
}
// For processes:
for (int p = 0; p < num_procs; p++) {
    wait(NULL);
}
```

### Performance Notes
- Use `clock_gettime(CLOCK_MONOTONIC, &ts)` for wall-clock timing:
```c
struct timespec start, end;
clock_gettime(CLOCK_MONOTONIC, &start);
// ... work ...
clock_gettime(CLOCK_MONOTONIC, &end);
double elapsed = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9;
```
- Prefer `MAP_SHARED` for mmap when child processes write to shared memory
- Memory-optimized version transposes matrix B for cache-friendly access
- Comment out `print_matrix()` calls before benchmarking

## Project Structure
```
src/
├── SecuentialMatrixSolver.c      # Basic sequential multiplication
├── MemoryMatrixSolver.c         # Cache-optimized (transposed B, -O3)
├── ThreadsMatrixSolver.c        # POSIX threads implementation
└── MultiprocessingMatrixSolver.c # Fork-based multiprocessing
output/                           # Compiled binaries (gitignored)
stats/                            # Timing results per machine
```

## Common Issues
- **Segfault**: Check matrix allocation/free pairing
- **Wrong results**: Verify row/column indices in multiplication loops
- **Timing includes printing**: Comment out `print_matrix()` before benchmarking
- **Thread overhead dominant**: Use large matrices (1000+) for meaningful measurements
