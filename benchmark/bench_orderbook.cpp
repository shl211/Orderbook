#include <array>
#include <iostream>
#include <cxxopts.hpp>

#include "driver.hpp"
#include "add_cancel_generator.hpp"
#include "order.hpp"
#include "matching/orderbook_list.hpp"
#include "matching/orderbook_vector.hpp"
#include "matching/orderbook_intrusive_list.hpp"

namespace ob = shl211::ob;
namespace bench = shl211::bench;

int main(int argc, char** argv) {
    cxxopts::Options options("bench_orderbook", "Orderbook benchmarks");

    options.add_options()
        ("n,iter", "Number of iterations", 
            cxxopts::value<std::size_t>()->default_value("5000000")) //5M
        ("w,warmup-iter", "Number of warmup iterations", 
            cxxopts::value<std::size_t>()->default_value("100000")) //100K
        ("v,verbose", "Verbose print output", 
            cxxopts::value<bool>()->default_value("false"))
        ("i,impl", "Implementation to benchmark: list|vector|intrusive|all", 
            cxxopts::value<std::string>()->default_value("all"))
        ("m,measurement", "Measuring with: timer|cycles",
            cxxopts::value<std::string>()->default_value("cycles"))
        ("h,help", "Print usage");
    
    auto result = options.parse(argc, argv);

    if(result.count("help")) {
        std::cout << options.help() << '\n';
        return 0;
    }
    

    bench::AddCancelGenerator gen{};
    const std::size_t WARMUP_ITERATIONS = result["iter"].as<std::size_t>();
    const std::size_t PERF_ITERATIONS = result["warmup-iter"].as<std::size_t>();
    const bool IS_VERBOSE_OUT = result["verbose"].as<bool>();

    const std::string impl = result["impl"].as<std::string>();
    const auto runList = 
        impl == "list" || impl == "all";
    const auto runVector = 
        impl == "vector" || impl == "all";
    const auto runIntrusive = 
        impl == "intrusive" || impl == "all";

    const std::string measurement = result["measurement"].as<std::string>();
    const bench::DriverTimer measureType = measurement == "cycles" ? 
        bench::DriverTimer::CyclesCpu : bench::DriverTimer::SteadyClock;

        if(runList)
    {
        ob::MatchingOrderBookListImpl book{};
        bench::OrderBookBenchmark<ob::MatchingOrderBookListImpl> benchmark{WARMUP_ITERATIONS, PERF_ITERATIONS, measureType};
    
        std::cout << "LIST IMPL\n";
        benchmark.run(book, gen);
        benchmark.report(IS_VERBOSE_OUT);
        benchmark.exportCsv("LATENCY_LIST_IMPL.csv");
        std::cout << "Outputting LATENCY_LIST_IMPL.csv\n";
    }
    
    if(runVector)
    {
        ob::MatchingOrderBookVectorImpl book2{};
        bench::OrderBookBenchmark<ob::MatchingOrderBookVectorImpl> benchmark2{WARMUP_ITERATIONS, PERF_ITERATIONS, measureType};
        std::cout << "VECTOR IMPL\n";
        benchmark2.run(book2, gen);
        benchmark2.report(IS_VERBOSE_OUT);
        benchmark2.exportCsv("LATENCY_VECTOR_IMPL.csv");
        std::cout << "Outputting LATENCY_VECTOR_IMPL.csv\n";
    }

    if(runIntrusive)
    {
        ob::MatchingOrderBookIntrusiveListImpl book3{};
        bench::OrderBookBenchmark<ob::MatchingOrderBookIntrusiveListImpl> benchmark3{WARMUP_ITERATIONS, PERF_ITERATIONS, measureType};
        std::cout << "INTRUSIVE LIST IMPL\n";
        benchmark3.run(book3, gen);
        benchmark3.report(IS_VERBOSE_OUT);
        benchmark3.exportCsv("LATENCY_INTRUSIVE_LIST_IMPL.csv");
            std::cout << "Outputting LATENCY_INTRUSIVE_LIST_IMPL.csv\n";
    }
}
