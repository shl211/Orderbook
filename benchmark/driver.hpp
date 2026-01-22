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

enum class DriverTimer {
    SteadyClock,
    CyclesCpu
};

namespace detail {
    inline uint64_t measureSteady(auto&& fn) {
        auto t0 = bench::steady_clock();
        fn();
        auto t1 = bench::steady_clock();
        return static_cast<uint64_t>(
            std::chrono::duration_cast<std::chrono::nanoseconds>(t1 - t0).count()
        );
    }

    inline uint64_t measureCycles(auto&& fn) {
        auto t0 = bench::rdtsc();
        fn();
        auto t1 = bench::rdtsc();
        return t1 - t0;
    }
}

template <ob::MatchingOrderBook Book>
class OrderBookBenchmark {
public:
    OrderBookBenchmark(size_t warmup, size_t iterations, DriverTimer record = DriverTimer::SteadyClock)
        : warmup_(warmup), iterations_(iterations), record_(record) {}

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
        switch (record_) {
        case DriverTimer::SteadyClock:
            latencies_.reserve(iterations_);
            for (size_t i = 0; i < iterations_; ++i) {
                auto& e = events[warmup_ + i];

                auto t0 = bench::steady_clock();
                applyEvent(book, e);
                auto t1 = bench::steady_clock();

                latencies_.push_back(
                    std::chrono::duration_cast<std::chrono::nanoseconds>(t1 - t0).count()
                );
            }
            break;

        case DriverTimer::CyclesCpu:
            latencies_.reserve(iterations_);
            for (size_t i = 0; i < iterations_; ++i) {
                auto& e = events[warmup_ + i];

                auto t0 = bench::rdtsc();
                applyEvent(book, e);
                auto t1 = bench::rdtsc();

                latencies_.push_back(t1 - t0);
            }
            break;
        }
    }

    void report(bool printOutput = true) {
        auto p = computePercentiles(latencies_);

        std::string units = record_ == DriverTimer::SteadyClock ?
            "ns" : "cycles";

        if(printOutput) {
            std::cout << std::format("p50: {} {}\n", p.p50, units);
            std::cout << std::format("p90: {} {}\n", p.p90, units);
            std::cout << std::format("p99: {} {}\n", p.p99, units);
            std::cout << std::format("p99.9: {} {}\n", p.p999, units);
            std::cout << std::format("max: {} {}\n", p.max, units);
        }
    }

    void exportCsv(const std::string& filename) const {
        std::ofstream out(filename);
        if(!out.is_open()) {
        std::cerr << std::format("Failed to open CSV file: {}\n", filename);            
        return;
        }

        std::string header = record_ == DriverTimer::SteadyClock ?
            "latency_ns" : "latency_cycles";

        out << std::format("{}\n", header);//header

        for(uint64_t c : latencies_) {
            out << c << "\n";
        }

        out.close();
    }

private:
    size_t warmup_;
    size_t iterations_;
    std::vector<uint64_t> latencies_;
    DriverTimer record_;
};

}

#endif
