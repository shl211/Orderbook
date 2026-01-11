#ifndef SHL211_BENCH_ADD_CANCEL_GENERATOR_HPP
#define SHL211_BENCH_ADD_CANCEL_GENERATOR_HPP

#include <vector>
#include <cstdlib>
#include <ctime>
#include "order.hpp"
#include "event.hpp"
#include "matching/orderbook_concept.hpp"

namespace shl211::bench {
struct AddCancelGenerator {
    ob::OrderId nextId{1};
    std::vector<ob::OrderId> live;

    int cancelProb = 30;
    int bidProb    = 50;
    int minPrice   = 95;
    int maxPrice   = 105;
    int minQty     = 1;
    int maxQty     = 10'000;

    AddCancelGenerator() {
        std::srand(static_cast<unsigned>(std::time(nullptr)));
    }

    Event generate() {
        // cancel event
        if (!live.empty() && (std::rand() % 100) < cancelProb) {
            ob::OrderId id = live.back();
            live.pop_back();
            return Event{
                EventType::Cancel,
                id,
                ob::Side::Buy,     // unused
                ob::Price{0},      // unused
                ob::Quantity{0}    // unused
            };
        }

        // add event
        ob::Side side =
            (std::rand() % 100) < bidProb ? ob::Side::Buy : ob::Side::Sell;

        int priceInt = minPrice + (std::rand() % (maxPrice - minPrice + 1));
        unsigned int qtyInt   = minQty   + (std::rand() % (maxQty - minQty + 1));

        ob::Price price{ priceInt };
        ob::Quantity qty{ qtyInt };

        ob::OrderId id{ nextId++ };
        live.push_back(id);

        return Event{
            EventType::Add,
            id,
            side,
            price,
            qty
        };
    }
};


}

#endif