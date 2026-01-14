#include <array>
#include <iostream>

#include "driver.hpp"
#include "add_cancel_generator.hpp"
#include "order.hpp"
#include "matching/orderbook_list.hpp"
#include "matching/orderbook_vector.hpp"

namespace ob = shl211::ob;
namespace bench = shl211::bench;

int main() {
    bench::AddCancelGenerator gen{};
    constexpr int WARMUP_ITERATIONS = 100'000;
    constexpr int PERF_ITERATIONS = 5'000'000;
    
    {
        ob::MatchingOrderBookListImpl book{};
        bench::OrderBookBenchmark<ob::MatchingOrderBookListImpl> benchmark{WARMUP_ITERATIONS, PERF_ITERATIONS};
    
        std::cout << "LIST IMPL\n";
        benchmark.run(book, gen);
        benchmark.report();
        benchmark.exportCsv("LATENCY_LIST_IMPL.csv");
    }
    
    {
        ob::MatchingOrderBookVectorImpl book2{};
        bench::OrderBookBenchmark<ob::MatchingOrderBookVectorImpl> benchmark2{WARMUP_ITERATIONS, PERF_ITERATIONS};
        std::cout << "VECTOR IMPL\n";
        benchmark2.run(book2, gen);
        benchmark2.report();
        benchmark2.exportCsv("LATENCY_VECTOR_IMPL.csv");
    }
}
