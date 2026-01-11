add_executable(bench_orderbook bench_orderbook.cpp)

target_compile_options(bench_orderbook PRIVATE -O2 -march=native -DNDEBUG)

target_include_directories(bench_orderbook
    INTERFACE
        ${PROJECT_SOURCE_DIR}/benchmark
)

target_link_libraries(bench_orderbook
    PRIVATE
        orderbook-lib
)