#ifndef SHL211_OB_MARKET_ORDERBOOK_CONCEPT_HPP
#define SHL211_OB_MARKET_ORDERBOOK_CONCEPT_HPP

#include <cstdint>
#include <optional>
#include <concepts>
#include <vector>

#include "order.hpp"
#include "shadow/orderbook_utils.hpp"

namespace shl211::ob {

template <typename Book>
concept ShadowOrderBook = 
requires(Book book,
        const Book& cbook, 
        const AddEvent& add,
        const ModifyEvent& modify,
        const CancelEvent& cancel,
        const TradeEvent& trade,
        std::size_t depth)
{
    { book.apply(add) } -> std::same_as<void>;
    { book.apply(modify) } -> std::same_as<void>;
    { book.apply(cancel) } -> std::same_as<void>;
    { book.apply(trade) } -> std::same_as<void>;

    { cbook.bestBid() } -> std::same_as<std::optional<Price>>;
    { cbook.bestAsk() } -> std::same_as<std::optional<Price>>;
    
    { cbook.bids(depth) } -> std::same_as<std::vector<ShadowPriceLevelSummary>>;
    { cbook.asks(depth) } -> std::same_as<std::vector<ShadowPriceLevelSummary>>;
};
}


#endif