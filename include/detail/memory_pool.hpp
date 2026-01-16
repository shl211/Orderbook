#ifndef SHL211_OB_DETAIL_MEMORY_POOL_HPP
#define SHL211_OB_DETAIL_MEMORY_POOL_HPP

#include <vector>
#include "order_node.hpp"

namespace shl211::ob::detail {

struct NodeWrapper : ob::OrderNode {
    NodeWrapper* next_free{ nullptr };
};

class OrderNodePool {
public:
    explicit OrderNodePool(std::size_t capacity)
        : storage_(capacity), free_head_(nullptr) 
    {
        for(std::size_t i = 0; i < capacity; ++i) {
            storage_[i].next_free = free_head_;
            free_head_ = &storage_[i];
        }
    }

    OrderNode* allocate() {
        if(!free_head_) {
            return nullptr;
        }

        NodeWrapper* n = free_head_;
        free_head_ = free_head_->next_free;

        n->next = nullptr;
        n->prev = nullptr;

        return static_cast<OrderNode*>(n);
    }

    void deallocate(OrderNode* node) {
        auto* n = static_cast<NodeWrapper*>(node);
        n->next_free = free_head_;
        free_head_ = n;
    }

private:
    std::vector<NodeWrapper> storage_;
    NodeWrapper* free_head_{ nullptr };
};


}

#endif