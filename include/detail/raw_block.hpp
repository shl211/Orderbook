#ifndef SHL211_OB_DETAIL_RAW_BLOCK_HPP
#define SHL211_OB_DETAIL_RAW_BLOCK_HPP

#include <cstddef>
#include <new>
#include <memory>
#include <type_traits>

namespace shl211::ob {

template <typename T>
class RawBlock {
public:
    explicit RawBlock(std::size_t capacity)
        : capacity_(capacity),
        data_(::operator new[](capacity * sizeof(T), std::align_val_t{alignof(T)}))
    {}

    ~RawBlock() {
        ::operator delete[](data_, std::align_val_t{alignof(T)});
    }

    RawBlock(const RawBlock&) = delete;
    RawBlock& operator=(const RawBlock&) = delete;
    RawBlock(RawBlock&&) = delete;
    RawBlock& operator=(RawBlock&&) = delete;

    void* slot(std::size_t index) noexcept {
        return static_cast<std::byte*>(data_) + index * sizeof(T);
    }

    const void* slot(std::size_t index) const noexcept {
        return static_cast<const std::byte*>(data_) + index * sizeof(T);
    }

    std::size_t capacity() const noexcept { return capacity_; }

private:
    std::size_t capacity_;
    void* data_;
};
}

#endif