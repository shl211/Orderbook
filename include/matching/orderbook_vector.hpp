#ifndef SHL211_MATCHING_ORDERBOOK_VECTOR_HPP
#define SHL211_MATCHING_ORDERBOOK_VECTOR_HPP

#include <vector>
#include <optional>
#include <algorithm>
#include <numeric>
#include <unordered_map>

#include "order.hpp"
#include "matching/orderbook_concept.hpp"
#include "detail/matching_orderbook_utils.hpp"
#include "detail/lazy_pop_front_vector.hpp"

namespace shl211::ob {

class MatchingOrderBookVectorImpl {
public:
    [[nodiscard]] AddResult add(Order order) noexcept;
    [[nodiscard]] bool cancel(OrderId id) noexcept;
    [[nodiscard]] bool modify(OrderId id, Quantity newQty) noexcept;
    [[nodiscard]] bool modify(OrderId id, Quantity newQty, Price newPrice) noexcept;

    [[nodiscard]] std::optional<Price> bestBid() const noexcept;
    [[nodiscard]] std::optional<Price> bestAsk() const noexcept;

    [[nodiscard]] Quantity bidSizeAt(Price price) const noexcept;
    [[nodiscard]] Quantity askSizeAt(Price price) const noexcept;

    [[nodiscard]] bool empty() const noexcept;

    [[nodiscard]] std::vector<PriceLevelSummary> bids(std::size_t depth) const noexcept;
    [[nodiscard]] std::vector<PriceLevelSummary> asks(std::size_t depth) const noexcept;

private:
    struct OrderLocation {
        Price price;
        Side side;
    };

    std::unordered_map<OrderId, OrderLocation> idToLocation_;

    struct LevelInternal {
        Price price;
        Quantity totalQuantity;
        detail::LazyPopFrontVector<Order> orders; //assume sorted
    };

    //assume sorted with best at back
    std::vector<LevelInternal> bids_;
    std::vector<LevelInternal> asks_;

    struct MatchResult {
        std::vector<ob::MatchResult> matches;
        Quantity filledAmount;
        Order remainingOrder;
    };

    [[nodiscard]] MatchResult match(const Order& order) noexcept;
    [[nodiscard]] bool canMatch(const Order& order) const noexcept;
};

static_assert(MatchingOrderBook<MatchingOrderBookVectorImpl>);

/* -------------------------------------------------------------- */
/*  IMPLEMENTATION  */

bool MatchingOrderBookVectorImpl::canMatch(const Order& order) const noexcept {
    const Price orderPrice = detail::processOrderPrice(order);
    const Quantity orderSize = order.getRemainingQuantity();
    const TimeInForce orderTif = order.getTimeInForce();
    const Side side = order.getSide();

    const bool requiresPartialMatch = orderTif == TimeInForce::GTC || orderTif == TimeInForce::IOC;
    const bool requiresFullMatch = orderTif == TimeInForce::FOK;

    const auto bestAskOpt = bestAsk();
    const auto bestBidOpt = bestBid();
    const bool hasMatchingAsk = side == Side::Buy && bestAskOpt && bestAskOpt.value() <= orderPrice;
    const bool hasMatchingBid = side == Side::Sell && bestBidOpt && bestBidOpt.value() >= orderPrice;

    if(requiresPartialMatch) {
        return hasMatchingAsk || hasMatchingBid;
    }

    if(requiresFullMatch) {
        if(side == Side::Buy) {
            auto rit = std::partition_point(asks_.rbegin(), asks_.rend(),
                [orderPrice](const LevelInternal& l) {
                    return l.price <= orderPrice;
                }
            );
            
            const Quantity liquidity = std::accumulate(asks_.rbegin(), rit, Quantity{0},
                [](Quantity sum, const auto& level) { return sum + level.totalQuantity; }
            );

            return liquidity >= orderSize;
        }
        else {
            auto rit = std::partition_point(bids_.rbegin(), bids_.rend(), 
                [orderPrice](const LevelInternal& l) {
                    return l.price >= orderPrice; 
                }
            );

            const Quantity liquidity = std::accumulate(bids_.rbegin(), rit, Quantity{0},
                [](Quantity sum, const auto& level) { return sum + level.totalQuantity; }
            );

            return liquidity >= orderSize;
        }
    }

    return false;
}

inline MatchingOrderBookVectorImpl::MatchResult MatchingOrderBookVectorImpl::match(const Order& order) noexcept {
    const Side side = order.getSide();
    const Price price = detail::processOrderPrice(order);
    const OrderId id = order.getOrderId();
    const Quantity desiredQty = order.getRemainingQuantity();
    Quantity remainingQtyToFill = desiredQty;

    std::vector<ob::MatchResult> matches;
    if(side == Side::Buy) {
        auto bestAskPriceOpt = bestAsk();
        while(bestAskPriceOpt.has_value() && 
            price >= bestAskPriceOpt.value() && remainingQtyToFill > Quantity{ 0 }
        ) {
            LevelInternal& level = asks_.back();

            while(!level.orders.empty() && level.orders.front().isFilled()) {
                idToLocation_.erase(level.orders.front().getOrderId());
                level.orders.pop_front();
            }

            if(level.orders.empty()) {
                asks_.pop_back();
                bestAskPriceOpt = bestAsk();
                continue;
            }

            //front() is guaranteed valid at this point
            Order& matchingOrder = level.orders.front();
            const Quantity matchedQty = matchingOrder.applyFill(remainingQtyToFill);
            remainingQtyToFill -= matchedQty;
            level.totalQuantity -= matchedQty;

            matches.emplace_back(
                matchingOrder.getOrderId(),
                matchedQty,
                level.price
            );

            if(matchingOrder.isFilled()) {
                idToLocation_.erase(matchingOrder.getOrderId());
                level.orders.pop_front();
            }

            if(level.totalQuantity == Quantity{ 0 }) {
                asks_.pop_back();
                bestAskPriceOpt = bestAsk();
            }
        }
    }
    else {
        auto bestBidPriceOpt = bestBid();
        while(bestBidPriceOpt.has_value() &&
            bestBidPriceOpt.value() >= price && remainingQtyToFill > Quantity{ 0 }
        ) {
            LevelInternal& level = bids_.back();

            while(!level.orders.empty() && level.orders.front().isFilled()) {
                idToLocation_.erase(level.orders.front().getOrderId());
                level.orders.pop_front();
            }

            if(level.orders.empty()) {
                bids_.pop_back();
                bestBidPriceOpt = bestBid();
                continue;
            }

            //front() is guaranteed valid at this point
            Order& matchingOrder = level.orders.front();
            const Quantity matchedQty = matchingOrder.applyFill(remainingQtyToFill);
            remainingQtyToFill -= matchedQty;
            level.totalQuantity -= matchedQty;

            matches.emplace_back(
                matchingOrder.getOrderId(),
                matchedQty,
                level.price
            );

            if(matchingOrder.isFilled()) {
                idToLocation_.erase(matchingOrder.getOrderId());
                level.orders.pop_front();
            }

            if(level.totalQuantity == Quantity{ 0 }) {
                bids_.pop_back();
                bestBidPriceOpt = bestBid();
            }
        }
    }

    Quantity filledQty = desiredQty - remainingQtyToFill;
    Order remainingOrder{ order };
    remainingOrder.applyFill(filledQty);

    return MatchingOrderBookVectorImpl::MatchResult{
        .matches = std::move(matches),
        .filledAmount = filledQty,
        .remainingOrder = std::move(remainingOrder)
    };
}

inline AddResult MatchingOrderBookVectorImpl::add(Order order) noexcept {
    AddResult result;

    if(canMatch(order)) {
        auto matches = match(order);
        result.matches = std::move(matches.matches);
        order.applyFill(matches.filledAmount);
    }
    
    const Side side = order.getSide();
    const Price price = detail::processOrderPrice(order);
    const Quantity size = order.getRemainingQuantity();
    const OrderId id = order.getOrderId();

    if(detail::shouldAddToBook(order)) {
        if(side == Side::Buy) {
            auto levelIt = std::lower_bound(bids_.begin(), bids_.end(), price,
                [](const LevelInternal& level, Price val) {
                    return level.price < val;
                }
            );

            //case1: price level already exists
            if(levelIt != bids_.end() && levelIt->price == price) {
                levelIt->totalQuantity += size;
                levelIt->orders.push_back(std::move(order));
            }
            //case 2: new price level needed
            else {
                bids_.insert(levelIt, LevelInternal {
                    .price = price,
                    .totalQuantity = size,
                    .orders = {std::move(order)}
                });
            }
        }
        else {
            auto levelIt = std::lower_bound(asks_.begin(), asks_.end(), price, 
                [](const LevelInternal& level, Price price) {
                    return level.price > price;
                }
            );

            if(levelIt != asks_.end() && levelIt->price == price) {
                levelIt->totalQuantity += size;
                levelIt->orders.push_back(std::move(order));
            }
            else {
                asks_.insert(levelIt, LevelInternal {
                    .price = price,
                    .totalQuantity = size, 
                    .orders = {std::move(order)}
                });
            }
        }

        idToLocation_.insert_or_assign(id, MatchingOrderBookVectorImpl::OrderLocation{price, side});
        result.remaining = id;
    }

    return result;
}

inline bool MatchingOrderBookVectorImpl::cancel(OrderId id) noexcept {
    auto itMap = idToLocation_.find(id);
    if(itMap == idToLocation_.end())
        return false;

    const auto [price, side] = itMap->second;
    idToLocation_.erase(itMap);

    auto& levels = (side == Side::Buy) ? bids_ : asks_;

    auto levelIt = std::lower_bound(levels.begin(), levels.end(), price, 
        [side](const LevelInternal& l, Price p) {
            return l.price > p;
    });

    if (levelIt != levels.end() && levelIt->price == price) {
        // Linear search for the ID
        auto orderIt = std::find_if(levelIt->orders.begin(), levelIt->orders.end(),
            [id](const Order& o) { return o.getOrderId() == id; });

        if (orderIt != levelIt->orders.end()) {
            // THE LAZY STEP
            levelIt->totalQuantity -= orderIt->getRemainingQuantity();
            (void) orderIt->applyFill(orderIt->getRemainingQuantity());//mark as 0

            if(levelIt->totalQuantity == Quantity{ 0 }) {
                levels.erase(levelIt);
            }

            return true;
        }
    }

    return false;
}

inline bool MatchingOrderBookVectorImpl::modify(OrderId id, Quantity newQty) noexcept {
    auto itMap = idToLocation_.find(id);
        if (itMap == idToLocation_.end()) return false;

        const Side side = itMap->second.side;
        const Price price = itMap->second.price;
        const TimeInForce tif = TimeInForce::GTC; // Assuming GTC for limit modification

        // 1. Cancel the old one (Marks it as 0 quantity in the vector)
        if (!cancel(id)) return false;

        // 2. Re-create the order using your static factory
        auto updatedOrderOpt = Order::makeLimit(id, side, price, newQty, tif);
        
        if (updatedOrderOpt) {
            // 3. Re-add to the book
            (void)add(std::move(*updatedOrderOpt));
            return true;
        }

        return false;
}

inline bool MatchingOrderBookVectorImpl::modify(OrderId id, Quantity newQty, Price newPrice) noexcept {
    auto itMap = idToLocation_.find(id);
    if (itMap == idToLocation_.end()) return false;

    const Side side = itMap->second.side;
    const TimeInForce tif = TimeInForce::GTC; // Assuming GTC for limit modification

    // 1. Cancel the old one (Marks it as 0 quantity in the vector)
    if (!cancel(id)) return false;

    // 2. Re-create the order using your static factory
    auto updatedOrderOpt = Order::makeLimit(id, side, newPrice, newQty, tif);
    
    if (updatedOrderOpt) {
        // 3. Re-add to the book
        (void)add(std::move(*updatedOrderOpt));
        return true;
    }

    return false;
}

inline std::optional<Price> MatchingOrderBookVectorImpl::bestBid() const noexcept {
    return bids_.empty() ? std::nullopt : std::make_optional(bids_.back().price); 
}

inline std::optional<Price> MatchingOrderBookVectorImpl::bestAsk() const noexcept {
    return asks_.empty() ? std::nullopt : std::make_optional(asks_.back().price);
}

inline Quantity MatchingOrderBookVectorImpl::bidSizeAt(Price price) const noexcept {
    auto levelIt = std::lower_bound(bids_.cbegin(), bids_.cend(), price,
        [](const LevelInternal& level, Price val) {
            return level.price > val;
        }
    );

    if(levelIt != bids_.cend() && levelIt->price == price) {
        return levelIt->totalQuantity;
    }

    return Quantity{ 0 };
}

inline Quantity MatchingOrderBookVectorImpl::askSizeAt(Price price) const noexcept {
    auto levelIt = std::lower_bound(asks_.cbegin(), asks_.cend(), price,
        [](const LevelInternal& level, Price val) {
            return level.price > val;
        }
    );

    if(levelIt != asks_.cend() && levelIt->price == price) {
        return levelIt->totalQuantity;
    }

    return Quantity{ 0 };
}

inline bool MatchingOrderBookVectorImpl::empty() const noexcept {
    return idToLocation_.empty();
}

inline std::vector<PriceLevelSummary> MatchingOrderBookVectorImpl::bids(std::size_t depth) const noexcept {
    std::size_t levels = std::min(depth, bids_.size());
    
    std::vector<PriceLevelSummary> snapshot;
    snapshot.reserve(levels);

    std::size_t count{};
    for(auto it = bids_.rbegin(), end = bids_.rend();
            it != end && count < levels; ++it)
    {
        const Price price = it->price;
        const Quantity qty = it->totalQuantity;
        snapshot.emplace_back(price, qty);

        ++count;
    }

    return snapshot;
}

inline std::vector<PriceLevelSummary> MatchingOrderBookVectorImpl::asks(std::size_t depth) const noexcept {
    std::size_t levels = std::min(depth, asks_.size());
    
    std::vector<PriceLevelSummary> snapshot;
    snapshot.reserve(levels);

    std::size_t count{};
    for(auto it = asks_.rbegin(), end = asks_.rend();
            it != end && count < levels; ++it)
    {
        const Price price = it->price;
        const Quantity qty = it->totalQuantity;
        snapshot.emplace_back(price, qty);

        ++count;
    }

    return snapshot;
}

}


#endif