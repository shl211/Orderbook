#ifndef SHL211_OB_MATCHING_ORDERBOOK_CONCEPT_HPP
#define SHL211_OB_MATCHING_ORDERBOOK_CONCEPT_HPP

#include <memory>
#include <optional>
#include <cstdint>
#include <vector>
#include <ostream>

#include "order.hpp"
#include "matching/orderbook_utils.hpp"

namespace shl211::ob {

template <typename Book>
concept MatchingOrderBook = 
requires(Book book, const Book& cbook,  Order order, 
        OrderId id, Quantity qty, Price price, size_t depth,
        std::ostream& os) 
{
    { book.add(std::move(order)) } -> std::same_as<AddResult>;
    { book.cancel(id) } -> std::same_as<bool>;
    { book.modify(id, qty) } -> std::same_as<bool>;
    { book.modify(id, qty, price) } -> std::same_as<bool>;

    { book.bestBid() } -> std::same_as<std::optional<Price>>;
    { book.bestAsk() } -> std::same_as<std::optional<Price>>;
    { book.bids(depth) } -> std::same_as<std::vector<PriceLevelSummary>>;
    { book.asks(depth) } -> std::same_as<std::vector<PriceLevelSummary>>;

    { book.dump(os, depth) } -> std::same_as<void>;
};


}


#endif