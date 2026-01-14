#ifndef SHL211_OB_DETAIL_STRONG_TYPE_HPP
#define SHL211_OB_DETAIL_STRONG_TYPE_HPP

#include <type_traits>
#include <compare>
#include <cstddef>

namespace shl211::ob::detail {

//Forward declare Mixins
template <typename Derived> struct Additive;
template <typename Derived> struct Comparable;

//StrongType defineiiton
template <typename Tag>
concept StrongTag = std::is_empty_v<Tag>;

template <typename T, StrongTag Tag, template<typename> class... Mixins>
class StrongType 
    : public Mixins<StrongType<T, Tag, Mixins...>>...
{

public:
    using This = StrongType<T, Tag, Mixins...>;
    using UnderlyingType = T;

    constexpr StrongType() noexcept : value_{} {}
    constexpr explicit StrongType(T v) noexcept : value_(v) {}
    constexpr explicit operator T() const noexcept { return value_; }
    [[nodiscard]] T get() const noexcept { return value_; }
    
protected:
    constexpr T& ref() noexcept { return value_; }
    constexpr const T& ref() const noexcept { return value_; }

    //only add mixins that require reference
    friend struct Additive<This>;
private:
    T value_;
};

//Mixin definitions
template <typename Derived>
struct Additive {
    friend constexpr Derived operator-(const Derived& a, const Derived& b) noexcept {
        return Derived(a.get() - b.get());
    }

    constexpr Derived& operator-=(const Derived& other) noexcept {
        static_cast<Derived&>(*this).ref() -= other.get();
        return static_cast<Derived&>(*this);
    }

    friend constexpr Derived operator+(const Derived& a, const Derived& b) noexcept {
        return Derived(a.get() + b.get());
    }

    constexpr Derived& operator+=(const Derived& other) noexcept {
        auto& self = static_cast<Derived&>(*this);
        self.ref() += other.get();
        return self;
    }

// Prefix increment: ++obj
    constexpr Derived& operator++() noexcept {
        auto& self = static_cast<Derived&>(*this);
        ++self.ref();
        return self;
    }

    // Postfix increment: obj++
    constexpr Derived operator++(int) noexcept {
        auto& self = static_cast<Derived&>(*this);
        Derived temp = self;
        ++self.ref();
        return temp;
    }

    // Prefix decrement: --obj
    constexpr Derived& operator--() noexcept {
        auto& self = static_cast<Derived&>(*this);
        --self.ref();
        return self;
    }

    // Postfix decrement: obj--
    constexpr Derived operator--(int) noexcept {
        auto& self = static_cast<Derived&>(*this);
        Derived temp = self;
        --self.ref();
        return temp;
    }
};

template <typename Derived>
struct Comparable {
    friend constexpr bool operator==(
        const Derived& a,
        const Derived& b) noexcept
    {
        return a.get() == b.get();
    }

    friend constexpr auto operator<=>(
        const Derived& a,
        const Derived& b) noexcept
    {
        return a.get() <=> b.get();
    }
};


}
namespace std {
    template<
        typename T,
        shl211::ob::detail::StrongTag Tag,
        template<typename> class... Mixins
    >
        requires requires (const T& v) { std::hash<T>{}(v); }
    struct hash<shl211::ob::detail::StrongType<T, Tag, Mixins...>> {
        size_t operator()(const auto& v) const noexcept {
            return std::hash<T>{}(v.get());
        }
    };
}


#endif