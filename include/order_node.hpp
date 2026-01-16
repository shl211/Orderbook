#ifndef SHL211_OB_ORDER_NODE_HPP
#define SHL211_OB_ORDER_NODE_HPP

#include "order.hpp"

namespace shl211::ob {
struct OrderNode {
    OrderNode(Order order)
        : order(std::move(order)) {}

    Order order;
    OrderNode* next{ nullptr };
    OrderNode* prev{ nullptr };
};
}

#endif