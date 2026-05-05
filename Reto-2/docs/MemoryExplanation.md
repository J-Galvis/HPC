# Memory Optimization Proposals

## 1. Replace int Arrays with char or uint8_t
Each cell only ever holds the value 0 or 1. Right now each cell occupies 4 bytes as an int, meaning 75% of every array is wasted space. Switching to char or uint8_t immediately cuts the memory footprint of both road arrays to one quarter of their current size.
The impact is significant not just for RAM usage but for cache behaviour — a road of N=5000 cells currently takes 20KB per array, which may not fit in L1 cache. At 1 byte per cell it drops to 5KB, which fits comfortably in most L1 caches and means each step's working set is loaded once rather than repeatedly fetched from L2/L3.

## 2. Avoid Per-Thread malloc Inside the Loop
Currently each thread calls malloc and free every density iteration inside the parallel region. Heap allocation is a serialised operation — the allocator uses internal locks, meaning all threads contend on the same allocator mutex every iteration. For many density steps this becomes a measurable bottleneck.
The fix is to allocate each thread's road arrays once before the parallel region using omp_get_max_threads() to size a pool, then pass each thread its pre-allocated slice. The threads never touch the allocator again during the sweep.