#include <gtest/gtest.h>

#include "shadow/orderbook_naive.hpp"

namespace ob = shl211::ob;

//Helpers
inline void addBuy(ob::ShadowOrderBookNaiveImpl& book, ob::OrderId id, ob::Price price, ob::Quantity qty) {
    book.apply(ob::AddEvent{id, ob::Side::Buy, price, qty});
}

inline void addSell(ob::ShadowOrderBookNaiveImpl& book, ob::OrderId id, ob::Price price, ob::Quantity qty) {
    book.apply(ob::AddEvent{id, ob::Side::Sell, price, qty});
}

TEST(ShadowOrderBookNaive, AddOrdersAndBestBidAsk) {
    ob::ShadowOrderBookNaiveImpl book;

    addBuy(book, ob::OrderId{ 1 }, ob::Price{ 100 }, ob::Quantity{ 10 });
    addBuy(book, ob::OrderId{ 2 }, ob::Price{ 101 }, ob::Quantity{ 5 });
    addSell(book, ob::OrderId{ 3 }, ob::Price{ 102 }, ob::Quantity{ 7 });
    addSell(book, ob::OrderId{ 4 }, ob::Price{ 103 }, ob::Quantity{ 8 });

    // Check best bid/ask
    ASSERT_TRUE(book.bestBid().has_value());
    EXPECT_EQ(book.bestBid().value(), ob::Price{ 101 });

    ASSERT_TRUE(book.bestAsk().has_value());
    EXPECT_EQ(book.bestAsk().value(), ob::Price{ 102 });

    // Check depth
    auto bids = book.bids(2);
    auto asks = book.asks(2);

    ASSERT_EQ(bids.size(), 2);
    EXPECT_EQ(bids[0].price, ob::Price{ 101 });
    EXPECT_EQ(bids[1].price, ob::Price{ 100 } );

    ASSERT_EQ(asks.size(), 2);
    EXPECT_EQ(asks[0].price, ob::Price{ 102 });
    EXPECT_EQ(asks[1].price, ob::Price{ 103 });
}

TEST(ShadowOrderBookNaive, ModifyOrder) {
    ob::ShadowOrderBookNaiveImpl book;

    addBuy(book, ob::OrderId{ 1 }, ob::Price{ 100 }, ob:: Quantity{ 10 });
    addBuy(book, ob::OrderId{ 2 }, ob::Price{ 101 }, ob:: Quantity{ 5 });

    // Modify order 2: lower price, increase qty
    book.apply(ob::ModifyEvent{ob::OrderId{ 2 }, ob::Side::Buy, ob::Price{ 101 }, ob::Price{ 99 }, ob::Quantity{ 5 }, ob::Quantity{ 12 }});

    auto bids = book.bids(2);
    EXPECT_EQ(book.bestBid().value(), ob::Price{ 100 });
    EXPECT_EQ(bids[0].price, ob::Price{ 100 });
    EXPECT_EQ(bids[0].qty, ob::Quantity{ 10 });
    EXPECT_EQ(bids[1].price, ob::Price{ 99 });
    EXPECT_EQ(bids[1].qty, ob::Quantity{ 12 });
}

TEST(ShadowOrderBookNaive, CancelOrder) {
    ob::ShadowOrderBookNaiveImpl book;

    addBuy(book, ob::OrderId{ 1 }, ob::Price{ 100 }, ob::Quantity{ 10 });
    addBuy(book, ob::OrderId{ 2 }, ob::Price{ 101 }, ob::Quantity{ 5 });

    // Cancel best bid
    book.apply(ob::CancelEvent{ob::OrderId{ 2 }, ob::Side::Buy, ob::Price{ 101 }, ob::Quantity{ 5 }});
    EXPECT_EQ(book.bestBid().value(), ob::Price{ 100 });
}

TEST(ShadowOrderBookNaive, TradeEvent) {
    ob::ShadowOrderBookNaiveImpl book;

    addSell(book, ob::OrderId{ 3 }, ob::Price{ 102 }, ob::Quantity{ 7 });
    addSell(book, ob::OrderId{ 4 }, ob::Price{ 103 }, ob::Quantity{ 8 });

    // Partial fill
    book.apply(ob::TradeEvent{ob::OrderId{ 3 }, ob::Side::Sell, ob::Price{ 102 }, ob::Quantity{ 3 }});
    auto asks = book.asks(2);
    EXPECT_EQ(asks[0].price, ob::Price{ 102 });
    EXPECT_EQ(asks[0].qty, ob::Quantity{ 4 }); // 7 - 3

    // Full fill
    book.apply(ob::TradeEvent{ob::OrderId{ 3 }, ob::Side::Sell, ob::Price{ 102 }, ob::Quantity{ 4 }});
    asks = book.asks(2);
    EXPECT_EQ(asks[0].price, ob::Price{ 103 }); // 102 removed
}