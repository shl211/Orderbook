#ifndef SHL211_OB_DETAIL_STRONG_TYPE_HPP
#define SHL211_OB_DETAIL_STRONG_TYPE_HPP

#include <type_traits>

namespace shl211::ob::detail {

//Forward declare Mixins
template <typename Derived> struct Additive;
template <typename Derived> struct Comparable;

//StrongType defineiiton
template <typename Tag>
concept StrongTag = 
    std::is_empty_v<Tag> &&
    std::is_trivial_v<Tag>;

template <typename T, StrongTag Tag, template<typename> class... Mixins>
class StrongType 
    : public Mixins<StrongType<T, Tag, Mixins...>>...
{

public:
    using This = StrongType<T, Tag, Mixins...>;

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


#endif