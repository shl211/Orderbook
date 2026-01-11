#ifndef SHL211_OB_ORDER_HPP
#define SHL211_OB_ORDER_HPP

#include <cstdint>
#include <optional>
#include <functional>

#include "detail/strong_type.hpp"

namespace shl211::ob {

namespace detail {
    struct PriceTag{};
    struct QuantityTag{};
    struct OrderIdTag{};
}

using Price = detail::StrongType<int64_t, detail::PriceTag, detail::Additive, detail::Comparable>;
using Quantity = detail::StrongType<uint64_t, detail::QuantityTag, detail::Additive, detail::Comparable>;
using OrderId = detail::StrongType<uint64_t, detail::OrderIdTag, detail::Additive, detail::Comparable>;

enum class Side {
    Buy,
    Sell
};

enum class OrderType {
    Limit,
    Market
};

enum class TimeInForce {
    GTC,
    IOC,
    FOK
};

class Order {
public:
    [[nodiscard]] static std::optional<Order> makeLimit(
        OrderId id,
        Side side,
        Price price,
        Quantity qty,
        TimeInForce tif = TimeInForce::GTC) noexcept
    {
        if( qty == Quantity{ 0 } || price < Price{ 0 } )
            return std::nullopt;

        return Order{id, side, OrderType::Limit, tif, price, qty};
    }

    [[nodiscard]] static std::optional<Order> makeMarket(
        OrderId id,
        Side side,
        Quantity qty,
        TimeInForce tif = TimeInForce::IOC) noexcept
    {
        if( qty == Quantity{ 0 } )
            return std::nullopt;

        return Order{id, side, OrderType::Market, tif, std::nullopt, qty};
    }

    [[nodiscard]] OrderId getOrderId() const noexcept { return id_; }
    [[nodiscard]] OrderType getOrderType() const noexcept { return type_; }
    [[nodiscard]] TimeInForce getTimeInForce() const noexcept { return tif_; }
    [[nodiscard]] Side getSide() const noexcept { return side_; }
    [[nodiscard]] std::optional<Price> getPrice() const noexcept { return price_; } //only std::nullopt for market orders
    [[nodiscard]] Quantity getInitialQuantity() const noexcept { return initial_; }
    [[nodiscard]] Quantity getRemainingQuantity() const noexcept { return remaining_; }

    [[nodiscard]] bool isMarket() const noexcept { return type_ == OrderType::Market; }
    [[nodiscard]] bool isLimit() const noexcept { return type_ == OrderType::Limit; }
    
    [[nodiscard]] bool isFilled() const noexcept { return remaining_ == Quantity{ 0 }; } 
    //returns matched quantity
    Quantity applyFill(Quantity qty) noexcept {
        Quantity matched = qty > remaining_ ? remaining_ : qty;
        remaining_ -= matched;
        return matched;
    }
    void changeQuantity(Quantity newQty) noexcept { remaining_ = newQty; }

private:
    Order(OrderId id, Side side, OrderType type, TimeInForce tif, std::optional<Price> price, Quantity quantity)
        : id_( id ),
        side_( side ),
        type_( type ),
        tif_( tif ),
        price_( price ),
        initial_( quantity ),
        remaining_( quantity )
            {}

    const OrderId id_;
    const Side side_;
    const OrderType type_;
    const TimeInForce tif_;
    const std::optional<Price> price_;

    const Quantity initial_;
    Quantity remaining_;
};
}

#endif