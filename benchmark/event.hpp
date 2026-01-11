#pragma once

#include "order.hpp"

namespace shl211::bench {

enum class EventType{ Add, Cancel };

struct Event {
    EventType type;
    ob::OrderId id;
    ob::Side side;
    ob::Price price;
    ob::Quantity qty;
};

template <typename Book>
inline void applyEvent(Book& book, const Event& e) {
    switch (e.type) {
        case EventType::Add: {
            auto maybe = ob::Order::makeLimit(
                e.id, e.side, e.price, e.qty, ob::TimeInForce::GTC
            );
            if (maybe) {
                (void) book.add(std::move(*maybe));
            }
            break;
        }
        case EventType::Cancel:
            (void)book.cancel(e.id);
            break;
    }
}

}