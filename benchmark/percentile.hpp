#ifndef SHL211_BENCH_PERCENTILE_HPP
#define SHL211_BENCH_PERCENTILE_HPP

#include <vector>
#include <algorithm>
#include <cstdint>

namespace shl211::bench {

struct Percentiles {
    uint64_t p50, p90, p99, p999, max;
};

inline Percentiles computePercentiles(std::vector<uint64_t>& v) {
    std::sort(v.begin(), v.end());
    auto n = v.size();

    auto pick = [&](double q) {
        return v[static_cast<size_t>(q * n)];
    };

    return {
        pick(0.50),
        pick(0.90),
        pick(0.99),
        pick(0.999),
        v.back()
    };
}

}

#endif