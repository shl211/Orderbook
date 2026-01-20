#ifndef SHL211_BENCH_DRIVER
#define SHL211_BENCH_DRIVER

#include <vector>
#include <functional>
#include <iostream>
#include <format>
#include <string>
#include <fstream>

#include "matching/orderbook_concept.hpp"
#include "cycles.hpp"
#include "event.hpp"
#include "percentile.hpp"

namespace shl211::bench {

template <ob::MatchingOrderBook Book>
class OrderBookBenchmark {
public:
    OrderBookBenchmark(size_t warmup, size_t iterations)
        : warmup_(warmup), iterations_(iterations) {}

    template<typename Generator>
    void run(Book& book, Generator& gen) {
        //pregenerate events for replay
        std::vector<Event> events;
        events.reserve(warmup_ + iterations_);

        for(size_t i = 0; i < warmup_ + iterations_; ++i) {
            events.push_back(gen.generate());
        }

        //warmup
        for(size_t i = 0; i < warmup_; ++i) {
            applyEvent(book, events[i]);
        }
        
        //measurement
        latencies_.reserve(iterations_);
        for(size_t i = 0; i < iterations_; ++i) {
            auto& e = events[warmup_ + i];

            auto t0 = bench::steady_clock();
            applyEvent(book, e);
            auto t1 = bench::steady_clock();

            latencies_.push_back(static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::nanoseconds>(t1 - t0).count()));
        }
    }

    void report(bool printOutput = true) {
        auto p = computePercentiles(latencies_);

        if(printOutput) {
            std::cout << std::format("p50: {} ns\n", p.p50);
            std::cout << std::format("p90: {} ns\n", p.p90);
            std::cout << std::format("p99: {} ns\n", p.p99);
            std::cout << std::format("p999: {} ns\n", p.p999);
            std::cout << std::format("max: {} ns\n", p.max);
        }
    }

    void exportCsv(const std::string& filename) const {
        std::ofstream out(filename);
        if(!out.is_open()) {
        std::cerr << std::format("Failed to open CSV file: {}\n", filename);            
        return;
        }

        out << "latency_cycles\n";//header

        for(uint64_t c : latencies_) {
            out << c << "\n";
        }

        out.close();
    }

private:
    size_t warmup_;
    size_t iterations_;
    std::vector<uint64_t> latencies_;
};

}

#endif
