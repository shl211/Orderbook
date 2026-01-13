#include <gtest/gtest.h>

#include "matching/orderbook_naive.hpp"
#include "matching/orderbook_vector.hpp"

namespace ob = shl211::ob;

using OrderBookImplementations = ::testing::Types<
    ob::MatchingOrderBookNaive,
    ob::MatchingOrderBookVector
>;

template <ob::MatchingOrderBook T>
class OrderBookTest : public ::testing::Test {
protected:
    T book;
};

TYPED_TEST_SUITE(OrderBookTest, OrderBookImplementations);

/* -------------- Add orders with no matching -------------------------------*/

TYPED_TEST(OrderBookTest, AddSingleBuyOrder) {
    (void) this->book.add(*ob::Order::makeLimit(
        ob::OrderId{ 1 },
        ob::Side::Buy,
        ob::Price{ 100 },
        ob::Quantity{ 10 }
    ));

    EXPECT_TRUE(this->book.bestBid().has_value());
    EXPECT_EQ(this->book.bestBid().value(), ob::Price{ 100 });
    EXPECT_FALSE(this->book.bestAsk().has_value());
}

TYPED_TEST(OrderBookTest, AddMultipleBuyOrder) {
    (void) this->book.add(*ob::Order::makeLimit(
        ob::OrderId{ 1 },
        ob::Side::Buy,
        ob::Price{ 100 },
        ob::Quantity{ 10 }
    ));

    (void) this->book.add(*ob::Order::makeLimit(
        ob::OrderId{ 2 },
        ob::Side::Buy,
        ob::Price{ 100 },
        ob::Quantity{ 20 }
    ));

    (void) this->book.add(*ob::Order::makeLimit(
        ob::OrderId{ 3 },
        ob::Side::Buy,
        ob::Price{ 100 },
        ob::Quantity{ 30 }
    ));

    (void) this->book.add(*ob::Order::makeLimit(
        ob::OrderId{ 4 },
        ob::Side::Buy,
        ob::Price{ 90 },
        ob::Quantity{ 40 }
    ));

    EXPECT_TRUE(this->book.bestBid().has_value());
    EXPECT_EQ(this->book.bestBid().value(), ob::Price{ 100 });
    EXPECT_FALSE(this->book.bestAsk().has_value());

    auto bids = this->book.bids(2);
    EXPECT_EQ(bids.size(), 2);
    EXPECT_EQ(bids[0].price, ob::Price{ 100 });
    EXPECT_EQ(bids[0].quantity, ob::Quantity{ 60 });
    EXPECT_EQ(bids[1].price, ob::Price{ 90 });
    EXPECT_EQ(bids[1].quantity, ob::Quantity{ 40 });
}

TYPED_TEST(OrderBookTest, AddSellOrder) {
    (void) this->book.add(*ob::Order::makeLimit(
        ob::OrderId{ 1 },
        ob::Side::Sell,
        ob::Price{ 100 },
        ob::Quantity{ 10 }
    ));

    EXPECT_TRUE(this->book.bestAsk().has_value());
    EXPECT_EQ(this->book.bestAsk().value(), ob::Price{ 100 });
    EXPECT_FALSE(this->book.bestBid().has_value());
}

TYPED_TEST(OrderBookTest, AddMultipleSellOrder) {
    (void) this->book.add(*ob::Order::makeLimit(
        ob::OrderId{ 1 },
        ob::Side::Sell,
        ob::Price{ 100 },
        ob::Quantity{ 10 }
    ));

    (void) this->book.add(*ob::Order::makeLimit(
        ob::OrderId{ 2 },
        ob::Side::Sell,
        ob::Price{ 100 },
        ob::Quantity{ 20 }
    ));

    (void) this->book.add(*ob::Order::makeLimit(
        ob::OrderId{ 3 },
        ob::Side::Sell,
        ob::Price{ 100 },
        ob::Quantity{ 30 }
    ));

    (void) this->book.add(*ob::Order::makeLimit(
        ob::OrderId{ 4 },
        ob::Side::Sell,
        ob::Price{ 90 },
        ob::Quantity{ 40 }
    ));

    EXPECT_TRUE(this->book.bestAsk().has_value());
    EXPECT_EQ(this->book.bestAsk().value(), ob::Price{ 90 });
    EXPECT_FALSE(this->book.bestBid().has_value());

    auto asks = this->book.asks(2);
    EXPECT_EQ(asks.size(), 2);
    EXPECT_EQ(asks[0].price, ob::Price{ 90 });
    EXPECT_EQ(asks[0].quantity, ob::Quantity{ 40 });
    EXPECT_EQ(asks[1].price, ob::Price{ 100 });
    EXPECT_EQ(asks[1].quantity, ob::Quantity{ 60 });
}

TYPED_TEST(OrderBookTest, ZeroQuantityOrderNeverRests) {
    auto res = this->book.add(*ob::Order::makeLimit(
        ob::OrderId{ 1 }, 
        ob::Side::Buy, 
        ob::Price{ 100 }, 
        ob::Quantity{ 0 }
    ));
    EXPECT_FALSE(res.remaining.has_value());
    EXPECT_TRUE(this->book.empty());
}


/* ------------------ Cancel Orders ---------------------------------------- */

TYPED_TEST(OrderBookTest, CancelOrderExisting) {
    (void) this->book.add(*ob::Order::makeLimit(
        ob::OrderId{ 1 },
        ob::Side::Sell,
        ob::Price{ 100 },
        ob::Quantity{ 10 }
    ));
    
    (void) this->book.add(*ob::Order::makeLimit(
        ob::OrderId{ 2 },
        ob::Side::Sell,
        ob::Price{ 100 },
        ob::Quantity{ 10 }
    ));

    EXPECT_TRUE(this->book.cancel(ob::OrderId{ 1 }));
    EXPECT_EQ(this->book.bestAsk().value(), ob::Price{ 100 });
    EXPECT_EQ(this->book.askSizeAt(ob::Price{ 100 }), ob::Quantity{ 10 });
    
    EXPECT_TRUE(this->book.cancel(ob::OrderId{ 2 }));
    EXPECT_FALSE(this->book.bestAsk().has_value());
    EXPECT_EQ(this->book.askSizeAt(ob::Price{ 100 }), ob::Quantity{ 0 });
}

TYPED_TEST(OrderBookTest, CancelOrderNonExisting) {
    EXPECT_FALSE(this->book.cancel( ob::OrderId{ 999 }));
}

/* --------------------- Matching ------------------------------------------ */

TYPED_TEST(OrderBookTest, LimitOrderMatch) {
    //set up resting this->book
    (void) this->book.add(*ob::Order::makeLimit(
        ob::OrderId{1},
        ob::Side::Buy,
        ob::Price{ 100 },
        ob::Quantity{ 50 }
    ));
    
    //should match
    auto res = this->book.add(*ob::Order::makeLimit(
        ob::OrderId{2},
        ob::Side::Sell,
        ob::Price{ 100 },
        ob::Quantity{ 30 }
    ));

    EXPECT_EQ(res.matches.size(), 1);
    EXPECT_EQ(res.matches[0].executionPrice, ob::Price{ 100 });
    EXPECT_EQ(res.matches[0].matched, ob::Quantity{ 30 });
    EXPECT_EQ(res.matches[0].restingOrderId, ob::OrderId{ 1 });
    EXPECT_FALSE(res.remaining.has_value());

    auto restingOnBook = this->book.bids(1);
    EXPECT_EQ(restingOnBook.size(), 1);
    EXPECT_EQ(restingOnBook[0].price, ob::Price{ 100 });
    EXPECT_EQ(restingOnBook[0].quantity, ob::Quantity{ 20 });
}

TYPED_TEST(OrderBookTest, LimitOrderMultipleSameLevelMatch) {
    //set up resting this->book
    (void) this->book.add(*ob::Order::makeLimit(
        ob::OrderId{1},
        ob::Side::Buy,
        ob::Price{ 100 },
        ob::Quantity{ 50 }
    ));

    (void) this->book.add(*ob::Order::makeLimit(
        ob::OrderId{2},
        ob::Side::Buy,
        ob::Price{ 100 },
        ob::Quantity{ 50 }
    ));
    
    //should match
    auto res = this->book.add(*ob::Order::makeLimit(
        ob::OrderId{3},
        ob::Side::Sell,
        ob::Price{ 100 },
        ob::Quantity{ 70 }
    ));

    EXPECT_EQ(res.matches.size(), 2);
    EXPECT_EQ(res.matches[0].executionPrice, ob::Price{ 100 });
    EXPECT_EQ(res.matches[0].matched, ob::Quantity{ 50 });
    EXPECT_EQ(res.matches[0].restingOrderId, ob::OrderId{ 1 });
    EXPECT_EQ(res.matches[1].executionPrice, ob::Price{ 100 });
    EXPECT_EQ(res.matches[1].matched, ob::Quantity{ 20 });
    EXPECT_EQ(res.matches[1].restingOrderId, ob::OrderId{ 2 });
    EXPECT_FALSE(res.remaining.has_value());

    auto restingOnBook = this->book.bids(1);
    EXPECT_EQ(restingOnBook.size(), 1);
    EXPECT_EQ(restingOnBook[0].price, ob::Price{ 100 });
    EXPECT_EQ(restingOnBook[0].quantity, ob::Quantity{ 30 });
}

TYPED_TEST(OrderBookTest, LimitOrderMultipleLevelsMatch) {
    //set up resting this->book
    (void) this->book.add(*ob::Order::makeLimit(
        ob::OrderId{1},
        ob::Side::Buy,
        ob::Price{ 100 },
        ob::Quantity{ 50 }
    ));

    (void) this->book.add(*ob::Order::makeLimit(
        ob::OrderId{2},
        ob::Side::Buy,
        ob::Price{ 105 },
        ob::Quantity{ 50 }
    ));
    
    //should match
    auto res = this->book.add(*ob::Order::makeLimit(
        ob::OrderId{3},
        ob::Side::Sell,
        ob::Price{ 100 },
        ob::Quantity{ 70 }
    ));

    EXPECT_EQ(res.matches.size(), 2);
    EXPECT_EQ(res.matches[0].executionPrice, ob::Price{ 105 });
    EXPECT_EQ(res.matches[0].matched, ob::Quantity{ 50 });
    EXPECT_EQ(res.matches[0].restingOrderId, ob::OrderId{ 2 });
    EXPECT_EQ(res.matches[1].executionPrice, ob::Price{ 100 });
    EXPECT_EQ(res.matches[1].matched, ob::Quantity{ 20 });
    EXPECT_EQ(res.matches[1].restingOrderId, ob::OrderId{ 1 });
    EXPECT_FALSE(res.remaining.has_value());

    auto restingOnBook = this->book.bids(1);
    EXPECT_EQ(restingOnBook.size(), 1);
    EXPECT_EQ(restingOnBook[0].price, ob::Price{ 100 });
    EXPECT_EQ(restingOnBook[0].quantity, ob::Quantity{ 30 });
}

TYPED_TEST(OrderBookTest, LimitOrderConsumeAndAdd) {
    //set up resting this->book
    (void) this->book.add(*ob::Order::makeLimit(
        ob::OrderId{1},
        ob::Side::Buy,
        ob::Price{ 100 },
        ob::Quantity{ 50 }
    ));
    
    //should match
    auto res = this->book.add(*ob::Order::makeLimit(
        ob::OrderId{2},
        ob::Side::Sell,
        ob::Price{ 100 },
        ob::Quantity{ 70 }
    ));

    EXPECT_EQ(res.matches.size(), 1);
    EXPECT_EQ(res.matches[0].executionPrice, ob::Price{ 100 });
    EXPECT_EQ(res.matches[0].matched, ob::Quantity{ 50 });
    EXPECT_EQ(res.matches[0].restingOrderId, ob::OrderId{ 1 });
    ASSERT_TRUE(res.remaining.has_value());
    EXPECT_EQ(res.remaining.value(), ob::OrderId{ 2 });

    auto bidsRestingOnBook = this->book.bids(1);
    ASSERT_TRUE(bidsRestingOnBook.empty());
    auto asksRestingOnBook = this->book.asks(1);
    EXPECT_EQ(asksRestingOnBook.size(), 1);
    EXPECT_EQ(asksRestingOnBook[0].price, ob::Price{ 100 });
    EXPECT_EQ(asksRestingOnBook[0].quantity, ob::Quantity{ 20 });
}

TYPED_TEST(OrderBookTest, MarketOrderIocConsumeMoreThanAvailable) {
    //set up resting this->book
    (void) this->book.add(*ob::Order::makeLimit(
        ob::OrderId{1},
        ob::Side::Sell,
        ob::Price{ 100 },
        ob::Quantity{ 50 }
    ));
    
    //should match
    auto res = this->book.add(*ob::Order::makeMarket(
        ob::OrderId{2},
        ob::Side::Buy,
        ob::Quantity{ 70 },
        ob::TimeInForce::IOC
    ));

    EXPECT_EQ(res.matches.size(), 1);
    EXPECT_EQ(res.matches[0].executionPrice, ob::Price{ 100 });
    EXPECT_EQ(res.matches[0].matched, ob::Quantity{ 50 });
    EXPECT_EQ(res.matches[0].restingOrderId, ob::OrderId{ 1 });
    EXPECT_FALSE(res.remaining.has_value());

    ASSERT_TRUE(this->book.empty());
}

TYPED_TEST(OrderBookTest, MarketOrderFOKInsufficientVolume) {
    //set up resting this->book
    (void) this->book.add(*ob::Order::makeLimit(
        ob::OrderId{1},
        ob::Side::Sell,
        ob::Price{ 100 },
        ob::Quantity{ 50 }
    ));
    
    //should match
    auto res = this->book.add(*ob::Order::makeMarket(
        ob::OrderId{2},
        ob::Side::Buy,
        ob::Quantity{ 70 },
        ob::TimeInForce::FOK
    ));

    ASSERT_TRUE(res.matches.empty());

    EXPECT_EQ(this->book.bestAsk().value(), ob::Price{ 100 });
    EXPECT_EQ(this->book.askSizeAt(ob::Price{ 100 }), ob::Quantity{ 50 });
}

TYPED_TEST(OrderBookTest, CrossedBookPrecention) {
    (void) this->book.add(*ob::Order::makeLimit(
        ob::OrderId{1},
        ob::Side::Sell,
        ob::Price{ 100 },
        ob::Quantity{ 50 }
    ));

    (void) this->book.add(*ob::Order::makeLimit(
        ob::OrderId{1},
        ob::Side::Buy,
        ob::Price{ 101 },
        ob::Quantity{ 50 }
    ));

    // Should match immediately, not rest
    EXPECT_FALSE(this->book.bestBid().has_value());
    EXPECT_FALSE(this->book.bestAsk().has_value());
}

TYPED_TEST(OrderBookTest, MarketOrderNoLiquidity) {
    auto res = this->book.add(*ob::Order::makeMarket(
        ob::OrderId{1},
        ob::Side::Sell,
        ob::Quantity{ 50 },
        ob::TimeInForce::IOC
    ));
    ASSERT_TRUE(res.matches.empty());
    EXPECT_FALSE(res.remaining.has_value());
}

TYPED_TEST(OrderBookTest, MarketFOKExactLiquidity) {
    (void)this->book.add(*ob::Order::makeLimit(
        ob::OrderId{ 1 }, 
        ob::Side::Sell, 
        ob::Price{ 100 },
        ob::Quantity{ 50 }
    ));

    auto res = this->book.add(*ob::Order::makeMarket(
        ob::OrderId{ 2 }, 
        ob::Side::Buy, 
        ob::Quantity{ 50 }, 
        ob::TimeInForce::FOK
    ));

    EXPECT_EQ(res.matches.size(), 1);
    EXPECT_FALSE(res.remaining.has_value());
    ASSERT_TRUE(this->book.empty());
}