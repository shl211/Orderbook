#ifndef SHL211_BENCH_CYCLES_HPP
#define SHL211_BENCH_CYCLES_HPP
#include <chrono>
#include <thread>
#include <cstdint>
#include <x86intrin.h>

namespace shl211::bench {
inline uint64_t rdtsc() {
    unsigned aux;
    return __rdtscp(&aux);
}

double measureTscGHz() {
    using namespace std::chrono;

    uint64_t t0 = __rdtscp(nullptr);
    auto start = steady_clock::now();

    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    uint64_t t1 = __rdtscp(nullptr);
    auto end = steady_clock::now();

    double cycles = double(t1 - t0);
    double seconds = duration<double>(end - start).count();

    return (cycles / seconds) / 1e9; // GHz
}

}

#endif