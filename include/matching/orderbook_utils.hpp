#ifndef SHL211_OB_MATCH_ORDERBOOK_UTILS_HPP
#define SHL211_OB_MATCH_ORDERBOOK_UTILS_HPP

#include <vector>
#include <optional>

#include "order.hpp"

namespace shl211::ob {

struct MatchResult {
    MatchResult(OrderId id, Quantity qty, Price price)
        : restingOrderId( id ), matched( qty ), executionPrice( price )
        {}

    OrderId restingOrderId;
    Quantity matched;
    Price executionPrice;
};

struct AddResult {
    bool accepted;
    std::vector<MatchResult> matches;
    std::optional<OrderId> remaining;
};

struct PriceLevelSummary {
    Price price; 
    Quantity quantity; 
};

}


#endif