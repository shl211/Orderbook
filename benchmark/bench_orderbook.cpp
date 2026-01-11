#include "driver.hpp"

#include "add_cancel_generator.hpp"
#include "order.hpp"
#include "matching/orderbook_naive.hpp"

namespace ob = shl211::ob;
namespace bench = shl211::bench;

int main() {
    bench::AddCancelGenerator gen{};
    ob::MatchingOrderBookNaive book{};

    bench::OrderBookBenchmark<ob::MatchingOrderBookNaive> benchmark{10'000,1'000'000};

    benchmark.run(book, gen);
    benchmark.report();
    benchmark.exportCsv("DATA.csv");
}
