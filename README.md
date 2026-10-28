# CPU Performance Simulator (Early Working Version)

A small C++ trace-driven simulator for exploring cache behavior and branch-prediction accuracy.

## Implemented
- Configurable set-associative cache with LRU replacement.
- Read/write memory access traces (`R address` or `W address`, address in decimal or `0x` hex).
- Bimodal and gshare branch predictors with configurable table size.
- Hit/miss rate and predictor accuracy / MPKI summary.

## Not implemented yet
Cycle-accurate CPU execution, multi-level caches, trace collection from the RISC-V CPU, and automated parameter-sweep plots are planned extensions.

## Build
```sh
g++ -std=c++17 -O2 -Wall -Wextra -pedantic src/main.cpp -o cpu-sim
```

## Run
```sh
./cpu-sim cache traces/memory.trace 64 2 16
./cpu-sim predict traces/branches.trace bimodal 64
./cpu-sim predict traces/branches.trace gshare 64
```

Cache arguments: `cache <trace> <capacity_bytes> <ways> <block_bytes>`.
Predictor arguments: `predict <trace> <bimodal|gshare> <table_entries>`.

Trace formats:
- Memory: one access per line, `R 0x1000` or `W 4096`.
- Branch: one outcome per line, `T` (taken) or `N` (not taken).
