#ifndef SHL211_OB_DETAIL_LAZY_POP_FRONT_VECTOR_HPP
#define SHL211_OB_DETAIL_LAZY_POP_FRONT_VECTOR_HPP

#include <vector>
#include <memory>
#include <initializer_list>

namespace shl211::ob::detail {

template <typename T>
class LazyPopFrontVector {
public:
    LazyPopFrontVector() = default;
    LazyPopFrontVector(std::initializer_list<T> init)
        : data_(init), frontIndex_(0) {}

    [[nodiscard]] bool empty() const noexcept { return frontIndex_ >= data_.size(); }
    [[nodiscard]] size_t size() const noexcept{ return empty() ? 0 : data_.size() - frontIndex_; }
    void reserve(std::size_t n) { data_.reserve(n); }

    T& front() noexcept { return data_[frontIndex_]; }
    const T& front() const noexcept { return data_[frontIndex_]; }

    void push_back(T value) {
        data_.push_back(std::move(value));
    }

    template <typename... Args>
    void emplace_back(Args&&... args) {
        data_.emplace_back(std::forward<Args>(args)...);
    }

    void pop_front() noexcept {
        if(!empty()) {
            frontIndex_++;
            maybeCompact();
        }
    }

    auto begin() noexcept { return data_.begin() + frontIndex_; }
    auto end() noexcept{ return data_.end(); }
    auto begin() const noexcept { return data_.begin() + frontIndex_; }
    auto end() const noexcept{ return data_.end(); }
    auto cbegin() const noexcept { return data_.cbegin() + frontIndex_; }
    auto cend() const noexcept{ return data_.cend(); }
    auto rbegin() noexcept { return data_.rbegin() + frontIndex_; }
    auto rend() noexcept{ return data_.rend(); }

private:
    std::vector<T> data_;
    std::size_t frontIndex_{0};

    void maybeCompact() {
        if(frontIndex_ > 64 && frontIndex_ * 2 > data_.size()) {
            std::vector<T> newData(data_.begin() + frontIndex_, data_.end());
            data_.swap(newData);
            frontIndex_ = 0;
        }
    }
};
}

#endif