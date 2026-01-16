#ifndef SHL211_OB_MATCHING_ORDERBOOK_INTRUSIVE_LIST_HPP
#define SHL211_OB_MATCHING_ORDERBOOK_INTRUSIVE_LIST_HPP

#include <map>
#include <unordered_map>
#include <numeric>

#include "order_node.hpp"
#include "matching/orderbook_concept.hpp"
#include "matching/orderbook_utils.hpp"
#include "detail/matching_orderbook_utils.hpp"
#include "detail/object_pool.hpp"

namespace shl211::ob {

class MatchingOrderBookIntrusiveListImpl {
public:
    explicit MatchingOrderBookIntrusiveListImpl(std::size_t poolSize = 4096)
        : memoryPool_(poolSize) {}
    
    MatchingOrderBookIntrusiveListImpl(const MatchingOrderBookIntrusiveListImpl&) = delete;
    MatchingOrderBookIntrusiveListImpl& operator=(const MatchingOrderBookIntrusiveListImpl&) = delete;
    MatchingOrderBookIntrusiveListImpl(MatchingOrderBookIntrusiveListImpl&&) = default;
    MatchingOrderBookIntrusiveListImpl& operator=(MatchingOrderBookIntrusiveListImpl&&) = default;

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
    struct PriceLevelInfo {
        OrderNode* orderHead{};
        OrderNode* orderTail{};
        Quantity liquidity{ 0 };
    };

    std::map<Price, PriceLevelInfo> asks_; 
    std::map<Price, PriceLevelInfo, std::greater<Price>> bids_;

    struct OrderLocation {
        Side side;
        Price price;
        OrderNode* location;
    };

    std::unordered_map<OrderId, OrderLocation> ordersById_;

    struct MatchResult {
        std::vector<ob::MatchResult> matches;
        Quantity filledAmount;
        Order remainingOrder;
    };

    [[nodiscard]] OrderNode* addToPool(const Order& order) noexcept;
    void removeFromPool(OrderNode* location) noexcept;
    [[nodiscard]] MatchResult match(const Order& order) noexcept;
    [[nodiscard]] bool canMatch(const Order& order) const noexcept;

    static void unlinkNode(PriceLevelInfo& level, OrderNode* node) noexcept;

    detail::ObjectPool<OrderNode> memoryPool_{ 4096 };
};

static_assert(MatchingOrderBook<MatchingOrderBookIntrusiveListImpl>);

/* -------------------------------------------------------------- */
/*  IMPLEMENTATION  */

inline OrderNode* MatchingOrderBookIntrusiveListImpl::addToPool(const Order& order) noexcept {
    return memoryPool_.allocate(order);
}

inline void MatchingOrderBookIntrusiveListImpl::removeFromPool(OrderNode* location) noexcept {
    memoryPool_.deallocate(location);
}

inline MatchingOrderBookIntrusiveListImpl::MatchResult MatchingOrderBookIntrusiveListImpl::match(const Order& order) noexcept {
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
            OrderNode* matchingOrder = info.orderHead;

            const Quantity matchedQty = matchingOrder->order.applyFill(remainingQtyToFill);
            remainingQtyToFill -= matchedQty;
            info.liquidity -= matchedQty;
            matches.emplace_back(matchingOrder->order.getOrderId(), matchedQty, matchPrice);

            if(matchingOrder->order.isFilled()) {
                ordersById_.erase(matchingOrder->order.getOrderId());
                unlinkNode(info, matchingOrder);
                removeFromPool(matchingOrder);

                if(!info.orderHead) {
                    asks_.erase(matchPrice);
                }
            }

            //setup next loop
            bestAskPriceOpt = bestAsk();
        }
    }
    else {
        auto bestBidPriceOpt = bestBid();
        while(bestBidPriceOpt.has_value() &&
            price <= bestBidPriceOpt.value() && remainingQtyToFill > Quantity{ 0 }
        ) {
            const Price matchPrice = bestBidPriceOpt.value();
            auto& info = bids_.find(matchPrice)->second;
            OrderNode* matchingOrder = info.orderHead;

            const Quantity matchedQty = matchingOrder->order.applyFill(remainingQtyToFill);
            remainingQtyToFill -= matchedQty;
            info.liquidity -= matchedQty;
            matches.emplace_back(matchingOrder->order.getOrderId(), matchedQty, matchPrice);

            if(matchingOrder->order.isFilled()) {
                ordersById_.erase(matchingOrder->order.getOrderId());
                unlinkNode(info, matchingOrder);
                removeFromPool(matchingOrder);

                if(!info.orderHead) {
                    bids_.erase(matchPrice);
                }
            }

            //setup next loop
            bestBidPriceOpt = bestBid();
        }
    }

    Quantity filledQty = desiredQty - remainingQtyToFill;
    Order remainingOrder{ order };
    remainingOrder.applyFill(filledQty);

    return MatchingOrderBookIntrusiveListImpl::MatchResult{
        .matches = std::move(matches),
        .filledAmount = filledQty,
        .remainingOrder = std::move(remainingOrder)
    };
}

inline bool MatchingOrderBookIntrusiveListImpl::canMatch(const Order& order) const noexcept {
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
            auto it = asks_.lower_bound(orderPrice);
            const Quantity liquidity = std::accumulate(asks_.begin(), it, Quantity{0},
            [](Quantity sum, const auto& level) { return sum + level.second.liquidity; });

            return liquidity >= orderSize;
        }
        else {
            auto it = bids_.begin();
            Quantity liquidity{};
            for (; it != bids_.end() && it->first >= orderPrice; ++it) {
                liquidity += it->second.liquidity;
            }
            return liquidity >= orderSize;
        }
    }

    return true;
}

inline void MatchingOrderBookIntrusiveListImpl::unlinkNode(PriceLevelInfo& level, OrderNode* node) noexcept {
    if(node->prev) {
        node->prev->next = node->next;
    }
    else {
        level.orderHead = node->next;
    }

    if(node->next) {
        node->next->prev = node->prev;
    }
    else {
        level.orderTail = node->prev;
    }
}

inline AddResult MatchingOrderBookIntrusiveListImpl::add(Order order) noexcept {
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
        OrderNode* orderNode = addToPool(order);
        
        if(side == Side::Buy) {
            auto& level = bids_[price];

            if(!level.orderHead) {
                level.orderHead = level.orderTail = orderNode;
            } 
            else {
                orderNode->prev = level.orderTail;
                level.orderTail->next = orderNode;
                level.orderTail = orderNode;
            }

            level.liquidity += size;
            ordersById_.emplace(
                id, 
                OrderLocation{ side, price, orderNode }
            );
        }
        else {
            auto& level = asks_[price];

            if(!level.orderHead) {
                level.orderHead = level.orderTail = orderNode;
            } 
            else {
                orderNode->prev = level.orderTail;
                level.orderTail->next = orderNode;
                level.orderTail = orderNode;
            }

            level.liquidity += size;
            ordersById_.emplace(
                id,
                OrderLocation{ side, price, orderNode }
            );
        }

        result.remaining = id;
    }

    return result;
}

inline bool MatchingOrderBookIntrusiveListImpl::cancel(OrderId id) noexcept {
    auto it = ordersById_.find(id);
    if( it == ordersById_.end() ) {
        return false;
    }

    OrderNode* node = it->second.location;
    const Price price = it->second.price;
    const Side side = it->second.side;

    if(side == Side::Buy) {
        auto levelIt = bids_.find(price);
        unlinkNode(levelIt->second, node);
        levelIt->second.liquidity -= node->order.getRemainingQuantity();

        if(!levelIt->second.orderHead) {
            bids_.erase(levelIt);
        }
    }
    else {
        auto levelIt = asks_.find(price);
        unlinkNode(levelIt->second, node);
        levelIt->second.liquidity -= node->order.getRemainingQuantity();

        if(!levelIt->second.orderHead) {
            asks_.erase(levelIt);
        }
    }

    ordersById_.erase(it);
    removeFromPool(node);
    return true;
}

inline bool MatchingOrderBookIntrusiveListImpl::modify(OrderId id, Quantity newQty) noexcept {
    auto it = ordersById_.find(id);
    if(it == ordersById_.end()) {
        return false;
    }

    Order oldOrder = it->second.location->order;

    (void) cancel(id);

    oldOrder.changeQuantity(newQty);
    (void) add(std::move(oldOrder));

    return true;
}

inline bool MatchingOrderBookIntrusiveListImpl::modify(OrderId id, Quantity newQty, Price newPrice) noexcept {
    auto it = ordersById_.find(id);
    if (it == ordersById_.end()) {
        return false;
    }

    Order oldOrder = it->second.location->order;

    (void) cancel(id);

    Order newOrder = Order::makeLimit(
        oldOrder.getOrderId(),
        oldOrder.getSide(),
        newPrice,
        newQty,
        oldOrder.getTimeInForce()
    ).value(); 

    (void) add(std::move(newOrder));

    return true;
}

inline std::optional<Price> MatchingOrderBookIntrusiveListImpl::bestBid() const noexcept {
    return bids_.empty() ? std::nullopt : std::make_optional(bids_.begin()->first);
}

inline std::optional<Price> MatchingOrderBookIntrusiveListImpl::bestAsk() const noexcept {
    return asks_.empty() ? std::nullopt : std::make_optional(asks_.begin()->first);
}

inline Quantity MatchingOrderBookIntrusiveListImpl::bidSizeAt(Price price) const noexcept {
    auto it = bids_.find(price);
    if(it == bids_.end()) {
        return Quantity{};
    }

    return it->second.liquidity;
}

inline Quantity MatchingOrderBookIntrusiveListImpl::askSizeAt(Price price) const noexcept {
    auto it = asks_.find(price);
    if(it == asks_.end()) {
        return Quantity{};
    }

    return it->second.liquidity;
}

inline bool MatchingOrderBookIntrusiveListImpl::empty() const noexcept {
    return ordersById_.empty();
}

inline std::vector<PriceLevelSummary> MatchingOrderBookIntrusiveListImpl::bids(std::size_t depth) const noexcept {
    std::size_t levels = std::min(depth, bids_.size());
    
    std::vector<PriceLevelSummary> snapshot;
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

inline std::vector<PriceLevelSummary> MatchingOrderBookIntrusiveListImpl::asks(std::size_t depth) const noexcept {
    std::size_t levels = std::min(depth, asks_.size());
    
    std::vector<PriceLevelSummary> snapshot;
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


}

#endif