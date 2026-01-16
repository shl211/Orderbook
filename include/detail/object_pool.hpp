#ifndef SHL211_OB_DETAIL_OBJECT_POOL_HPP
#define SHL211_OB_DETAIL_OBJECT_POOL_HPP

#include <vector>

#include "detail/raw_block.hpp"

namespace shl211::ob::detail {

// ObjectPool<T> provides fast allocation/deallocation for POD or
// trivially destructible types only. Do not use for types with
// non-trivial destructors, or ensure all objects are deallocated
// before pool destruction.

template <typename T>
concept TriviallyDestructible = std::is_trivially_destructible_v<T>;

template <TriviallyDestructible T>
class ObjectPool {
public:
    explicit ObjectPool(std::size_t blockSize = 1024)
        : blockSize_(blockSize)
    {}

    ~ObjectPool() {
        for (auto block : blocks_) {
            delete block;
        }
    }

    ObjectPool(const ObjectPool&) = delete;
    ObjectPool& operator=(const ObjectPool&) = delete;

    ObjectPool(ObjectPool&& other) noexcept 
        : blocks_(std::move(other.blocks_)),
        freeList_(other.freeList_),
        blockSize_(other.blockSize_)
    {
        other.freeList_ = nullptr;
        other.blockSize_ = 0;
    }

    ObjectPool& operator=(ObjectPool&& other) noexcept {
        if (this != &other) {
            // Destroy current blocks
            for (auto block : blocks_) delete block;

            blocks_ = std::move(other.blocks_);
            freeList_ = other.freeList_;
            blockSize_ = other.blockSize_;

            other.freeList_ = nullptr;
            other.blockSize_ = 0;
        }
        return *this;
    }

    template <typename... Args>
    T* allocate(Args&&... args) {
        if(freeList_ == nullptr) {
            allocateBlock();
        }

        T* obj = reinterpret_cast<T*>(freeList_);
        freeList_ = reinterpret_cast<FreeNode*>(obj);
        freeList_ = freeList_->next;

        new (obj) T(std::forward<Args>(args)...);
        return obj;
    }

    void deallocate(T* obj) noexcept {
        if(!obj) return;

        obj->~T();

        reinterpret_cast<FreeNode*>(obj)->next = freeList_;
        freeList_ = reinterpret_cast<FreeNode*>(obj);
    }


private:
    struct FreeNode {
        FreeNode* next;
    };

    void allocateBlock() {
        RawBlock<T>* block = new RawBlock<T>(blockSize_);
        blocks_.push_back(block);

        for(std::size_t i = 0; i < blockSize_; ++i) {
            auto* slotPtr = reinterpret_cast<T*>(block->slot(i));
            deallocate(slotPtr);//push into free list
        }
    }

    std::vector<RawBlock<T>*> blocks_;
    FreeNode* freeList_{ nullptr };
    std::size_t blockSize_{};
};

}

#endif