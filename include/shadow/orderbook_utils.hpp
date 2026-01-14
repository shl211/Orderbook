#ifndef SHL211_OB_MARKET_ORDERBOOK_UTILS_HPP
#define SHL211_OB_MARKET_ORDERBOOK_UTILS_HPP

#include "order.hpp"

namespace shl211::ob {

struct AddEvent {
    OrderId id;
    Side side;
    Price price;
    Quantity qty;
};

struct ModifyEvent {
    OrderId id;
    Side side;
    Price oldPrice;
    Price newPrice;
    Quantity oldQty;
    Quantity newQty;
};

struct CancelEvent {
    OrderId id;
    Side side;
    Price price;
    Quantity qty;
};

struct TradeEvent {
    OrderId restingId;
    Side restingSide;
    Price price;
    Quantity qty;
};

struct ShadowPriceLevelSummary {
    Price price;
    Quantity qty;
};


}

#endif