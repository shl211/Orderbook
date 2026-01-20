# Orderbook Implementations

This project explores multiple implementations of a price–time priority (L3) matching order book, with a focus on data structure design, matching semantics, and performance characteristics under different conditions. The goal is not to build a production exchange, but to study how different data structure choices affect latency, memory access patterns, and algorithmic complexity in a matching engine context.

Implementations currently include:

- **List-based order book** - A straightforward implementation using STL-based structures, prioritising clarity and theoretically optimal algorithmic complexity.

- **Vector-based order book** - Uses contiguous storage to improve cache locality and iteration performance, at the cost of higher algorithmic complexity for certain operations.

- **Intrusive list order book** - Stores linkage directly within order objects, which are allocated from a memory pool, reducing dynamic memory allocation.

In addition, simplified L2-style order books are implemented to model aggregated order tracking as observed
from exchange market data feeds.

## Build

The project requires **CMake ≥ 3.30** and **C++20**.

To clone the repo

```sh
git clone git@github.com:shl211/Orderbook.git
```

To configure and build:

```sh
cmake -S . -B build [FLAGS] && cmake --build build
```

Tests can be run from the `build` directory with `ctest`.

### Build Flags:

- `-DBUILD_BENCH=ON`
    - Enables performance benchmarks
    - Default OFF
- `-DBUILD_TESTS=ON`
    - Enables unit tests
    - Default ON

# Performance Benchmarks

Currently the following benchmarks are available:

- `bench_orderbook`
    - Generates latency distribution for basic orderbook operations
    - 70/30 Add/Cancel order distribution
    - Generates a sorted latency distribution exported as csv file

Advanced options are available via the `-h` flag:

```sh
./benchmark/bench_orderbook -h
```

To run the benchmark from the `build` directory: 

```sh
./benchmark/bench_orderbook -v #Verbose output
python3 ../benchmark/analyse.py LATENCY_LIST_IMPL.csv LATENCY_VECTOR_IMPL.csv LATENCY_INTRUSIVE_LIST_IMPL.csv
```

To analyse the results from the `build` directory using the provided Python script:

```py
python3 -m pip install pandas numpy matplotlib
python3 ../benchmark/analyse.py \
    LATENCY_LIST_IMPL.csv \
    LATENCY_VECTOR_IMPL.csv \
    LATENCY_INTRUSIVE_LIST_IMPL.csv

```

- Notes:
    - Benchmarks are intended for relative comparisons between implementations, not absolute performance claims
    - For consistent results, run benchmarks on an isolated system with minimal backgruond load