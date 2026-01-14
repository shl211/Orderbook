#ifndef SHL211_OB_SHADOW_ORDERBOOK_LIST_HPP
#define SHL211_OB_SHADOW_ORDERBOOK_LIST_HPP

#include <optional>
#include <vector>
#include <map>
#include <unordered_map>

#include "order.hpp"
#include "shadow/orderbook_utils.hpp"
#include "shadow/orderbook_concept.hpp"

namespace shl211::ob {

class ShadowOrderBookNaiveImpl {
public:
    void apply(const AddEvent& e);
    void apply(const ModifyEvent& e);
    void apply(const CancelEvent& e);
    void apply(const TradeEvent& e);

    [[nodiscard]] std::optional<Price> bestBid() const noexcept;
    [[nodiscard]] std::optional<Price> bestAsk() const noexcept;

    [[nodiscard]] std::vector<ShadowPriceLevelSummary> bids(size_t depth) const noexcept;
    [[nodiscard]] std::vector<ShadowPriceLevelSummary> asks(size_t depth) const noexcept;

private:
    struct OrderState {
        Side side;
        Price price;
        Quantity qty;
    };

    std::map<Price, Quantity, std::greater<Price>> bids_;
    std::map<Price, Quantity> asks_;

    std::unordered_map<OrderId, OrderState> orders_;
};

static_assert(ShadowOrderBook<ShadowOrderBookNaiveImpl>);

/*
* IMPLEMENTATION
*/

void ShadowOrderBookNaiveImpl::apply(const AddEvent& e) {
    orders_.insert_or_assign(e.id, OrderState{e.side, e.price, e.qty});
    
    if(e.side == Side::Buy) {
        bids_[e.price] += e.qty;
    }
    else {
        asks_[e.price] += e.qty;
    }
}

void ShadowOrderBookNaiveImpl::apply(const ModifyEvent& e) {
    auto it = orders_.find(e.id);
    if(it == orders_.end()) 
        return;
        
    auto& st = it->second;

    if(st.side == Side::Buy) {
        bids_[st.price] -= st.qty;
        if(bids_[st.price] == Quantity{ 0 }) {
            bids_.erase(st.price);
        }
    }
    else {
        asks_[st.price] -= st.qty;
        if(asks_[st.price] == Quantity{ 0 }) {
            asks_.erase(st.price);
        }
    }

    st.price = e.newPrice;
    st.qty = e.newQty;

    if(st.side == Side::Buy) {
        bids_[st.price] += st.qty;
    }
    else {
        asks_[st.price] += st.qty;
    }
}

void ShadowOrderBookNaiveImpl::apply(const CancelEvent& e) {
    auto it = orders_.find(e.id);
    if(it == orders_.end())
        return;

    auto& st = it->second;
    orders_.erase(e.id);

    if(st.side == Side::Buy) {
        bids_[st.price] -= st.qty;
        if(bids_[st.price] == Quantity{0}) 
            bids_.erase(st.price);
    } else {
        asks_[st.price] -= st.qty;
        if(asks_[st.price] == Quantity{0}) 
            asks_.erase(st.price);
    }
}

void ShadowOrderBookNaiveImpl::apply(const TradeEvent& e) {
    auto it = orders_.find(e.restingId);
    if (it == orders_.end())
        return;

    auto& st = it->second;

    if(st.side == Side::Buy) {
        bids_[st.price] -= e.qty;
        st.qty -= e.qty;
        
        if(st.qty == Quantity{0}) 
            orders_.erase(e.restingId);
        
        if(bids_[st.price] == Quantity{0}) 
            bids_.erase(st.price);
    } else {
        asks_[st.price] -= e.qty;
        st.qty -= e.qty;

        if(st.qty == Quantity{0}) 
            orders_.erase(e.restingId);
        
        if(asks_[st.price] == Quantity{0}) 
            asks_.erase(st.price);
    }
}

std::optional<Price> ShadowOrderBookNaiveImpl::bestBid() const noexcept {
    if(bids_.empty()) return std::nullopt;
    return bids_.begin()->first;
}

std::optional<Price> ShadowOrderBookNaiveImpl::bestAsk() const noexcept {
    if(asks_.empty()) return std::nullopt;
    return asks_.begin()->first;
}

std::vector<ShadowPriceLevelSummary> ShadowOrderBookNaiveImpl::bids(size_t depth) const noexcept {
    std::vector<ShadowPriceLevelSummary> out;
    out.reserve(depth);
    size_t count = 0;
    for(const auto& [price, qty] : bids_) {
        out.push_back({price, qty});
        if(++count == depth) break;
    }
    return out;
}

std::vector<ShadowPriceLevelSummary> ShadowOrderBookNaiveImpl::asks(size_t depth) const noexcept {
    std::vector<ShadowPriceLevelSummary> out;
    out.reserve(depth);
    size_t count = 0;
    for(const auto& [price, qty] : asks_) {
        out.push_back({price, qty});
        if(++count == depth) break;
    }
    return out;
}

}
#endif