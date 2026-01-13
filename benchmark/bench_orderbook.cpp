#include <array>
#include <iostream>

#include "driver.hpp"
#include "add_cancel_generator.hpp"
#include "order.hpp"
#include "matching/orderbook_naive.hpp"
#include "matching/orderbook_vector.hpp"

namespace ob = shl211::ob;
namespace bench = shl211::bench;

int main() {
    bench::AddCancelGenerator gen{};
    ob::MatchingOrderBookNaive book{};
    bench::OrderBookBenchmark<ob::MatchingOrderBookNaive> benchmark{100'000,10'000'000};

    std::cout << "NAIVE IMPL\n";
    benchmark.run(book, gen);
    benchmark.report();
    benchmark.exportCsv("LATENCY_NAIVE_IMPL.csv");
    
    ob::MatchingOrderBookVector book2{};
    bench::OrderBookBenchmark<ob::MatchingOrderBookVector> benchmark2{10'000,1'000'000};
    
    std::cout << "VECTOR IMPL\n";
    benchmark2.run(book2, gen);
    benchmark2.report();
    benchmark2.exportCsv("LATENCY_VECTOR_IMPL.csv");
}
