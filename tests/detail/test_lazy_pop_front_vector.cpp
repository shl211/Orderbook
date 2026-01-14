#include "gtest/gtest.h"

#include "detail/lazy_pop_front_vector.hpp"

namespace detail = shl211::ob::detail;

TEST(LazyPopFrontVector, PopFront) {
    detail::LazyPopFrontVector<int> v;
    EXPECT_TRUE(v.empty());

    v.push_back(1);
    v.push_back(2);
    v.push_back(3);

    EXPECT_FALSE(v.empty());
    EXPECT_EQ(v.size(), 3);
    EXPECT_EQ(v.front(), 1);

    v.pop_front();
    EXPECT_EQ(v.front(), 2);
    EXPECT_EQ(v.size(), 2);

    v.pop_front();
    EXPECT_EQ(v.front(), 3);
    EXPECT_EQ(v.size(), 1);

    v.pop_front();
    EXPECT_TRUE(v.empty());
}