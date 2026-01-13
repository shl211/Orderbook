# Orderbook Implementations

I try different implementations of an L3 orderbook.

To build the project, need CMake >= 3.30 and C++20:

```sh
cmake -S . -B build [FLAGS] && cmake --build build
```

Tests can be run from the `build` directory with `ctest`.

# Performance Benchmarks

To compile with performance benchmarks, the flag `-DBUILD_BENCH=ON` can be used. To run the benchmark from the `build` directory: 

```sh
./benchmark/bench_orderbook
python3 ../benchmark/analyse.py DATA.csv
```

Note that for the Python you may also need the following libraries:

```py
pip install pandas numpy matplotlib
```