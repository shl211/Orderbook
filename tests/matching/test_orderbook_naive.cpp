#include <gtest/gtest.h>

#include "matching/orderbook_naive.hpp"

namespace ob = shl211::ob;

class NaiveOrderbookTest : public ::testing::Test {
protected:
    ob::MatchingOrderBookNaive book;
};

/* -------------- Add orders with no matching -------------------------------*/

TEST_F(NaiveOrderbookTest, AddSingleBuyOrder) {
    (void) book.add(*ob::Order::makeLimit(
        ob::OrderId{ 1 },
        ob::Side::Buy,
        ob::Price{ 100 },
        ob::Quantity{ 10 }
    ));

    EXPECT_TRUE(book.bestBid().has_value());
    EXPECT_EQ(book.bestBid().value(), ob::Price{ 100 });
    EXPECT_FALSE(book.bestAsk().has_value());
}

TEST_F(NaiveOrderbookTest, AddMultipleBuyOrder) {
    (void) book.add(*ob::Order::makeLimit(
        ob::OrderId{ 1 },
        ob::Side::Buy,
        ob::Price{ 100 },
        ob::Quantity{ 10 }
    ));

    (void) book.add(*ob::Order::makeLimit(
        ob::OrderId{ 2 },
        ob::Side::Buy,
        ob::Price{ 100 },
        ob::Quantity{ 20 }
    ));

    (void) book.add(*ob::Order::makeLimit(
        ob::OrderId{ 3 },
        ob::Side::Buy,
        ob::Price{ 100 },
        ob::Quantity{ 30 }
    ));

    (void) book.add(*ob::Order::makeLimit(
        ob::OrderId{ 4 },
        ob::Side::Buy,
        ob::Price{ 90 },
        ob::Quantity{ 40 }
    ));

    EXPECT_TRUE(book.bestBid().has_value());
    EXPECT_EQ(book.bestBid().value(), ob::Price{ 100 });
    EXPECT_FALSE(book.bestAsk().has_value());

    auto bids = book.bids(2);
    EXPECT_EQ(bids.size(), 2);
    EXPECT_EQ(bids[0].price, ob::Price{ 100 });
    EXPECT_EQ(bids[0].quantity, ob::Quantity{ 60 });
    EXPECT_EQ(bids[1].price, ob::Price{ 90 });
    EXPECT_EQ(bids[1].quantity, ob::Quantity{ 40 });
}

TEST_F(NaiveOrderbookTest, AddSellOrder) {
    (void) book.add(*ob::Order::makeLimit(
        ob::OrderId{ 1 },
        ob::Side::Sell,
        ob::Price{ 100 },
        ob::Quantity{ 10 }
    ));

    EXPECT_TRUE(book.bestAsk().has_value());
    EXPECT_EQ(book.bestAsk().value(), ob::Price{ 100 });
    EXPECT_FALSE(book.bestBid().has_value());
}

TEST_F(NaiveOrderbookTest, AddMultipleSellOrder) {
    (void) book.add(*ob::Order::makeLimit(
        ob::OrderId{ 1 },
        ob::Side::Sell,
        ob::Price{ 100 },
        ob::Quantity{ 10 }
    ));

    (void) book.add(*ob::Order::makeLimit(
        ob::OrderId{ 2 },
        ob::Side::Sell,
        ob::Price{ 100 },
        ob::Quantity{ 20 }
    ));

    (void) book.add(*ob::Order::makeLimit(
        ob::OrderId{ 3 },
        ob::Side::Sell,
        ob::Price{ 100 },
        ob::Quantity{ 30 }
    ));

    (void) book.add(*ob::Order::makeLimit(
        ob::OrderId{ 4 },
        ob::Side::Sell,
        ob::Price{ 90 },
        ob::Quantity{ 40 }
    ));

    EXPECT_TRUE(book.bestAsk().has_value());
    EXPECT_EQ(book.bestAsk().value(), ob::Price{ 90 });
    EXPECT_FALSE(book.bestBid().has_value());

    auto asks = book.asks(2);
    EXPECT_EQ(asks.size(), 2);
    EXPECT_EQ(asks[0].price, ob::Price{ 90 });
    EXPECT_EQ(asks[0].quantity, ob::Quantity{ 40 });
    EXPECT_EQ(asks[1].price, ob::Price{ 100 });
    EXPECT_EQ(asks[1].quantity, ob::Quantity{ 60 });
}

TEST_F(NaiveOrderbookTest, ZeroQuantityOrderNeverRests) {
    auto res = book.add(*ob::Order::makeLimit(
        ob::OrderId{ 1 }, 
        ob::Side::Buy, 
        ob::Price{ 100 }, 
        ob::Quantity{ 0 }
    ));
    EXPECT_FALSE(res.remaining.has_value());
    EXPECT_TRUE(book.empty());
}


/* ------------------ Cancel Orders ---------------------------------------- */

TEST_F(NaiveOrderbookTest, CancelOrderExisting) {
    (void) book.add(*ob::Order::makeLimit(
        ob::OrderId{ 1 },
        ob::Side::Sell,
        ob::Price{ 100 },
        ob::Quantity{ 10 }
    ));
    
    (void) book.add(*ob::Order::makeLimit(
        ob::OrderId{ 2 },
        ob::Side::Sell,
        ob::Price{ 100 },
        ob::Quantity{ 10 }
    ));

    EXPECT_TRUE(book.cancel(ob::OrderId{ 1 }));
    EXPECT_EQ(book.bestAsk().value(), ob::Price{ 100 });
    EXPECT_EQ(book.askSizeAt(ob::Price{ 100 }), ob::Quantity{ 10 });
    
    EXPECT_TRUE(book.cancel(ob::OrderId{ 2 }));
    EXPECT_FALSE(book.bestAsk().has_value());
    EXPECT_EQ(book.askSizeAt(ob::Price{ 100 }), ob::Quantity{ 0 });
}

TEST_F(NaiveOrderbookTest, CancelOrderNonExisting) {
    EXPECT_FALSE(book.cancel( ob::OrderId{ 999 }));
}

/* --------------------- Matching ------------------------------------------ */

TEST_F(NaiveOrderbookTest, LimitOrderMatch) {
    //set up resting book
    (void) book.add(*ob::Order::makeLimit(
        ob::OrderId{1},
        ob::Side::Buy,
        ob::Price{ 100 },
        ob::Quantity{ 50 }
    ));
    
    //should match
    auto res = book.add(*ob::Order::makeLimit(
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

    auto restingOnBook = book.bids(1);
    EXPECT_EQ(restingOnBook.size(), 1);
    EXPECT_EQ(restingOnBook[0].price, ob::Price{ 100 });
    EXPECT_EQ(restingOnBook[0].quantity, ob::Quantity{ 20 });
}

TEST_F(NaiveOrderbookTest, LimitOrderMultipleSameLevelMatch) {
    //set up resting book
    (void) book.add(*ob::Order::makeLimit(
        ob::OrderId{1},
        ob::Side::Buy,
        ob::Price{ 100 },
        ob::Quantity{ 50 }
    ));

    (void) book.add(*ob::Order::makeLimit(
        ob::OrderId{2},
        ob::Side::Buy,
        ob::Price{ 100 },
        ob::Quantity{ 50 }
    ));
    
    //should match
    auto res = book.add(*ob::Order::makeLimit(
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

    auto restingOnBook = book.bids(1);
    EXPECT_EQ(restingOnBook.size(), 1);
    EXPECT_EQ(restingOnBook[0].price, ob::Price{ 100 });
    EXPECT_EQ(restingOnBook[0].quantity, ob::Quantity{ 30 });
}

TEST_F(NaiveOrderbookTest, LimitOrderMultipleLevelsMatch) {
    //set up resting book
    (void) book.add(*ob::Order::makeLimit(
        ob::OrderId{1},
        ob::Side::Buy,
        ob::Price{ 100 },
        ob::Quantity{ 50 }
    ));

    (void) book.add(*ob::Order::makeLimit(
        ob::OrderId{2},
        ob::Side::Buy,
        ob::Price{ 105 },
        ob::Quantity{ 50 }
    ));
    
    //should match
    auto res = book.add(*ob::Order::makeLimit(
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

    auto restingOnBook = book.bids(1);
    EXPECT_EQ(restingOnBook.size(), 1);
    EXPECT_EQ(restingOnBook[0].price, ob::Price{ 100 });
    EXPECT_EQ(restingOnBook[0].quantity, ob::Quantity{ 30 });
}

TEST_F(NaiveOrderbookTest, LimitOrderConsumeAndAdd) {
    //set up resting book
    (void) book.add(*ob::Order::makeLimit(
        ob::OrderId{1},
        ob::Side::Buy,
        ob::Price{ 100 },
        ob::Quantity{ 50 }
    ));
    
    //should match
    auto res = book.add(*ob::Order::makeLimit(
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

    auto bidsRestingOnBook = book.bids(1);
    ASSERT_TRUE(bidsRestingOnBook.empty());
    auto asksRestingOnBook = book.asks(1);
    EXPECT_EQ(asksRestingOnBook.size(), 1);
    EXPECT_EQ(asksRestingOnBook[0].price, ob::Price{ 100 });
    EXPECT_EQ(asksRestingOnBook[0].quantity, ob::Quantity{ 20 });
}

TEST_F(NaiveOrderbookTest, MarketOrderIocConsumeMoreThanAvailable) {
    //set up resting book
    (void) book.add(*ob::Order::makeLimit(
        ob::OrderId{1},
        ob::Side::Sell,
        ob::Price{ 100 },
        ob::Quantity{ 50 }
    ));
    
    //should match
    auto res = book.add(*ob::Order::makeMarket(
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

    ASSERT_TRUE(book.empty());
}

TEST_F(NaiveOrderbookTest, MarketOrderFOKInsufficientVolume) {
    //set up resting book
    (void) book.add(*ob::Order::makeLimit(
        ob::OrderId{1},
        ob::Side::Sell,
        ob::Price{ 100 },
        ob::Quantity{ 50 }
    ));
    
    //should match
    auto res = book.add(*ob::Order::makeMarket(
        ob::OrderId{2},
        ob::Side::Buy,
        ob::Quantity{ 70 },
        ob::TimeInForce::FOK
    ));

    ASSERT_TRUE(res.matches.empty());

    EXPECT_EQ(book.bestAsk().value(), ob::Price{ 100 });
    EXPECT_EQ(book.askSizeAt(ob::Price{ 100 }), ob::Quantity{ 50 });
}

TEST_F(NaiveOrderbookTest, CrossedBookPrecention) {
    (void) book.add(*ob::Order::makeLimit(
        ob::OrderId{1},
        ob::Side::Sell,
        ob::Price{ 100 },
        ob::Quantity{ 50 }
    ));

    (void) book.add(*ob::Order::makeLimit(
        ob::OrderId{1},
        ob::Side::Buy,
        ob::Price{ 101 },
        ob::Quantity{ 50 }
    ));

    // Should match immediately, not rest
    EXPECT_FALSE(book.bestBid().has_value());
    EXPECT_FALSE(book.bestAsk().has_value());
}

TEST_F(NaiveOrderbookTest, MarketOrderNoLiquidity) {
    auto res = book.add(*ob::Order::makeMarket(
        ob::OrderId{1},
        ob::Side::Sell,
        ob::Quantity{ 50 },
        ob::TimeInForce::IOC
    ));
    ASSERT_TRUE(res.matches.empty());
    EXPECT_FALSE(res.remaining.has_value());
}

TEST_F(NaiveOrderbookTest, MarketFOKExactLiquidity) {
    (void)book.add(*ob::Order::makeLimit(
        ob::OrderId{ 1 }, 
        ob::Side::Sell, 
        ob::Price{ 100 },
        ob::Quantity{ 50 }
    ));

    auto res = book.add(*ob::Order::makeMarket(
        ob::OrderId{ 2 }, 
        ob::Side::Buy, 
        ob::Quantity{ 50 }, 
        ob::TimeInForce::FOK
    ));

    EXPECT_EQ(res.matches.size(), 1);
    EXPECT_FALSE(res.remaining.has_value());
    ASSERT_TRUE(book.empty());
}


