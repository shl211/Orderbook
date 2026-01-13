#ifndef SHL211_OB_DETAIL_MATCHING_ORDER_UTILS_HPP
#define SHL211_OB_DETAIL_MATCHING_ORDER_UTILS_HPP

#include <optional>

#include "order.hpp"

namespace shl211::ob::detail {
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

#endif