#include <gtest/gtest.h>

#include "order.hpp"

namespace ob = shl211::ob;

TEST(OrderTests, MakeLimitOrdersValid) {
    auto limitOrderQuery = ob::Order::makeLimit(
        ob::OrderId{ 1 },
        ob::Side::Buy,
        ob::Price{ 10 },
        ob::Quantity{ 100 },
        ob::TimeInForce::GTC
    );

    EXPECT_TRUE(limitOrderQuery.has_value());
    ob::Order limitOrder{ std::move(limitOrderQuery.value()) };

    EXPECT_EQ(limitOrder.getOrderId(), ob::OrderId{ 1 });
    EXPECT_EQ(limitOrder.getSide(), ob::Side::Buy);
    EXPECT_EQ(limitOrder.getOrderType(), ob::OrderType::Limit);
    EXPECT_EQ(limitOrder.getTimeInForce(), ob::TimeInForce::GTC);
    EXPECT_EQ(limitOrder.getPrice().value(), ob::Price{ 10 });
    EXPECT_EQ(limitOrder.getRemainingQuantity(), ob::Quantity{ 100 });
    EXPECT_EQ(limitOrder.getInitialQuantity(), ob::Quantity{ 100 });
}

TEST(OrderTests, MakeMarketOrdersValid) {
    auto marketOrderQuery = ob::Order::makeMarket(
        ob::OrderId{ 1 },
        ob::Side::Sell,
        ob::Quantity{ 100 },
        ob::TimeInForce::GTC
    );

    EXPECT_TRUE(marketOrderQuery.has_value());
    ob::Order marketOrder{ std::move(marketOrderQuery.value()) };

    EXPECT_EQ(marketOrder.getOrderId(), ob::OrderId{ 1 });
    EXPECT_EQ(marketOrder.getSide(), ob::Side::Sell);
    EXPECT_EQ(marketOrder.getOrderType(), ob::OrderType::Market);
    EXPECT_EQ(marketOrder.getTimeInForce(), ob::TimeInForce::GTC);
    EXPECT_FALSE(marketOrder.getPrice().has_value());
    EXPECT_EQ(marketOrder.getRemainingQuantity(), ob::Quantity{ 100 });
    EXPECT_EQ(marketOrder.getInitialQuantity(), ob::Quantity{ 100 });
}
