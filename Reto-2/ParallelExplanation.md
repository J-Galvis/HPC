# Little explanation of the optimization done for Parallelism

## Option 1 — Parallel Loop Over Cells (Most Obvious)

The inner loop that iterates over all N cells is the most natural target. Since every cell reads exclusively from the old array and writes exclusively to the new array, there is zero data conflict between threads. Each thread can own a contiguous chunk of cells and compute their new values simultaneously.

The velocity counter (moved_this_step) would be the only shared variable, requiring a reduction operation — each thread counts its own moved cars locally and they are summed at the end of each step, which OpenMP handles natively.
This is the lowest-hanging fruit and likely gives the biggest speedup since the cell loop is where essentially all the computation lives.

## Option 2 — Parallel Loop Over Density Values (Embarrassingly Parallel)

Looking one level higher, each density value in the outer loop is completely independent of every other. Simulating density 0.3 shares absolutely nothing with simulating density 0.7 — different road arrays, different random seeds, different results.

This is called embarrassingly parallel — no synchronization, no shared state, no reductions needed. You would assign different density values to different threads and they run their full simulations (warmup + measurement) entirely in isolation.

The tradeoff is memory: each thread needs its own pair of road arrays of size N, so memory usage scales with the number of threads. For N=5000 this is trivial, but for very large N it becomes a consideration.