#ifndef SHL211_OB_MATCHING_ORDERBOOK_NAIVE_HPP
#define SHL211_OB_MATCHING_ORDERBOOK_NAIVE_HPP

#include <optional>
#include <vector>
#include <list>
#include <map>
#include <unordered_map>
#include <limits>
#include <numeric>

#include "order.hpp"
#include "matching/orderbook_utils.hpp"
#include "matching/orderbook_concept.hpp"

namespace shl211::ob {

class MatchingOrderBookNaive {
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

    struct Level { 
        Price price; 
        Quantity quantity; 
    };

    [[nodiscard]] std::vector<Level> bids(std::size_t depth) const noexcept;
    [[nodiscard]] std::vector<Level> asks(std::size_t depth) const noexcept;

private:
    using PriceLevel = std::list<Order>;

    struct PriceLevelInfo {
        PriceLevel orderList{};
        Quantity liquidity{ 0 };
    };

    std::map<Price, PriceLevelInfo> asks_; 
    std::map<Price, PriceLevelInfo, std::greater<Price>> bids_;

    struct OrderLocation {
        Side side;
        Price price;
        PriceLevel::iterator location;
    };

    std::unordered_map<OrderId, OrderLocation> orderLocation_;

    struct MatchResult {
        std::vector<ob::MatchResult> matches;
        Quantity filledAmount;
        Order remainingOrder;
    };
    
    [[nodiscard]] MatchResult match(const Order& order) noexcept;
    [[nodiscard]] bool canMatch(const Order& order) const noexcept;
    bool cancelOrderHelper(OrderId id, bool subtractLiquidity);

};

static_assert(MatchingOrderBook<MatchingOrderBookNaive>);

/* -------------------------------------------------------------- */
/*  IMPLEMENTATION  */

namespace detail {
    constexpr Price MIN_PRICE{ 0 };
    constexpr Price MAX_PRICE{ std::numeric_limits<Price::UnderlyingType>::max() };

    inline Price processOrderPrice(const Order& order) {
        auto priceOpt = order.getPrice();

        if(priceOpt.has_value()) 
            return priceOpt.value();

        Side side = order.getSide();
        return side == Side::Buy ? MAX_PRICE : MIN_PRICE;
    }

    inline bool shouldAddToBook(const Order& order) {
        const OrderType type = order.getOrderType();
        const TimeInForce tif = order.getTimeInForce();
        const Quantity remainingQty = order.getRemainingQuantity();

        const bool hasRemaining = remainingQty > Quantity{0}; 
        const bool canSitOnBook = order.getTimeInForce() == TimeInForce::GTC; 
        
        return hasRemaining && canSitOnBook;
    }
}

bool MatchingOrderBookNaive::canMatch(const Order& order) const noexcept {
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
            auto it = asks_.upper_bound(orderPrice);
            const Quantity liquidity = std::accumulate(asks_.begin(), it, Quantity{0},
            [](Quantity sum, const auto& level) { return sum + level.second.liquidity; });

            return liquidity >= orderSize;
        }
        else {
            auto it = bids_.upper_bound(orderPrice);
            const Quantity liquidity = std::accumulate(bids_.begin(), it, Quantity{0},
            [](Quantity sum, const auto& level) { return sum + level.second.liquidity; });
    
            return liquidity >= orderSize;
        }
    }

    return true;
}

MatchingOrderBookNaive::MatchResult MatchingOrderBookNaive::match(const Order& order) noexcept {
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
            const Price matchPrice = bestAskPriceOpt.value();
            auto& info = asks_.find(matchPrice)->second;
            Order& matchingOrder = info.orderList.front();

            const Quantity matchedQty = matchingOrder.applyFill(remainingQtyToFill);
            remainingQtyToFill -= matchedQty;
            info.liquidity -= matchedQty;
            matches.emplace_back(matchingOrder.getOrderId(), matchedQty, matchPrice);

            if(matchingOrder.isFilled()) {
                cancelOrderHelper(matchingOrder.getOrderId(), false);
            }

            //set up next loop
            bestAskPriceOpt = bestAsk();
        }
    }
    else {
        auto bestBidPriceOpt = bestBid();
        while(bestBidPriceOpt.has_value() && 
            bestBidPriceOpt.value() >= price  && remainingQtyToFill > Quantity{ 0 }
        ) {
            const Price matchPrice = bestBidPriceOpt.value();
            auto& info = bids_.find(matchPrice)->second;
            Order& matchingOrder = info.orderList.front();
    
            const Quantity matchedQty = matchingOrder.applyFill(remainingQtyToFill);
            remainingQtyToFill -= matchedQty;
            info.liquidity -= matchedQty;
            matches.emplace_back(matchingOrder.getOrderId(), matchedQty, matchPrice);
    
            if(matchingOrder.isFilled()) {
                cancelOrderHelper(matchingOrder.getOrderId(), false);
            }
    
            //set up next loop
            bestBidPriceOpt = bestBid();
        }
    }

    Quantity filledQty = desiredQty - remainingQtyToFill;
    Order remainingOrder{ order };
    remainingOrder.applyFill(filledQty);

    return MatchingOrderBookNaive::MatchResult{
        .matches = std::move(matches),
        .filledAmount = filledQty,
        .remainingOrder = std::move(remainingOrder)
    };
}

bool MatchingOrderBookNaive::cancelOrderHelper(OrderId id, bool subtractLiquidity) {
    auto it = orderLocation_.find(id);
    if(it == orderLocation_.end()) {
        return false;
    }

    const OrderLocation& locationInfo = it->second;
    if(locationInfo.side == Side::Buy) {
        const Price orderPrice = detail::processOrderPrice(*locationInfo.location);
        
        auto& info = bids_[locationInfo.price];
        info.orderList.erase(locationInfo.location);
        
        if(info.orderList.empty())
            bids_.erase(orderPrice);
        
        if(subtractLiquidity) {
            const Quantity orderSize = locationInfo.location->getRemainingQuantity();
            info.liquidity -= orderSize;
        }
    }
    else {
        const Price orderPrice = detail::processOrderPrice(*locationInfo.location);
        
        auto& info = asks_[locationInfo.price];
        info.orderList.erase(locationInfo.location);
        
        if(info.orderList.empty())
            asks_.erase(orderPrice);
        
        if(subtractLiquidity) {
            const Quantity orderSize = locationInfo.location->getRemainingQuantity();
            info.liquidity -= orderSize;
        }
    }

    return static_cast<bool>(orderLocation_.erase(id));
}

inline AddResult MatchingOrderBookNaive::add(Order order) noexcept {
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
            PriceLevelInfo& priceLevelInfo = bids_[price];
            PriceLevel& orderList = priceLevelInfo.orderList;
            priceLevelInfo.liquidity += size;
            auto it = orderList.emplace(orderList.end(), std::move(order));
            orderLocation_.emplace(id, OrderLocation{side, price, it});
        }
        else {
            PriceLevelInfo& priceLevelInfo = asks_[price];
            PriceLevel& orderList = priceLevelInfo.orderList;
            priceLevelInfo.liquidity += size;
            auto it = orderList.emplace(orderList.end(), std::move(order));
            orderLocation_.emplace(id, OrderLocation{side, price, it});
        }

        result.remaining = id;
    }

    return result;
}

inline bool MatchingOrderBookNaive::cancel(OrderId id) noexcept {
    return cancelOrderHelper(id, true);
}

inline bool MatchingOrderBookNaive::modify(OrderId id, Quantity newQty) noexcept {
    auto it = orderLocation_.find(id);
    if(it == orderLocation_.end()) {
        return false;
    }

    auto loc = it->second;
    Order oldOrder = *loc.location;
    cancelOrderHelper(id, true);

    oldOrder.changeQuantity(newQty);
    (void)add(std::move(oldOrder));
    
    return true;
}

inline bool MatchingOrderBookNaive::modify(OrderId id, Quantity newQty, Price newPrice) noexcept {
    auto it = orderLocation_.find(id);
    if (it == orderLocation_.end())
        return false;

    auto loc = it->second;
    Order oldOrder = *loc.location;

    cancelOrderHelper(id, true);

    Order newOrder = Order::makeLimit(
        oldOrder.getOrderId(),
        oldOrder.getSide(),
        newPrice,
        newQty,
        oldOrder.getTimeInForce()
    ).value();
    (void)add(std::move(newOrder));

    return true;
}


inline std::optional<Price> MatchingOrderBookNaive::bestBid() const noexcept {
    if(bids_.empty())
        return std::nullopt;

    return bids_.begin()->first;
}

inline std::optional<Price> MatchingOrderBookNaive::bestAsk() const noexcept {
    if(asks_.empty())
        return std::nullopt;
    
    return asks_.begin()->first;
}

inline bool MatchingOrderBookNaive::empty() const noexcept {
    return orderLocation_.empty();
}

inline std::vector<MatchingOrderBookNaive::Level> MatchingOrderBookNaive::bids(std::size_t depth) const noexcept {
    std::size_t levels = std::min(depth, bids_.size());
    
    std::vector<MatchingOrderBookNaive::Level> snapshot;
    snapshot.reserve(levels);

    std::size_t count{};
    for(auto it = bids_.begin(), end = bids_.end();
            it != end && count < levels; ++it)
    {
        const Price price = it->first;
        const Quantity qty = it->second.liquidity;
        snapshot.emplace_back(price, qty);

        ++count;
    }

    return snapshot;
}

inline std::vector<MatchingOrderBookNaive::Level> MatchingOrderBookNaive::asks(std::size_t depth) const noexcept {
    std::size_t levels = std::min(depth, asks_.size());
    
    std::vector<MatchingOrderBookNaive::Level> snapshot;
    snapshot.reserve(levels);

    std::size_t count{};
    for(auto it = asks_.begin(), end = asks_.end();
            it != end && count < levels; ++it)
    {
        const Price price = it->first;
        const Quantity qty = it->second.liquidity;
        snapshot.emplace_back(price, qty);

        ++count;
    }

    return snapshot;
}

inline Quantity MatchingOrderBookNaive::bidSizeAt(Price price) const noexcept {
    auto it = bids_.find(price);

    if(it == bids_.end())
        return Quantity{ 0 }; 

    return it->second.liquidity;
}

inline Quantity MatchingOrderBookNaive::askSizeAt(Price price) const noexcept {
    auto it = asks_.find(price);

    if(it == asks_.end())
        return Quantity{ 0 }; 

    return it->second.liquidity;
}
}

#endif