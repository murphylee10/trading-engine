#include "catch.hpp"
#include "../src/OrderBook.h"

TEST_CASE("Single limit order rests without match", "[OrderBook]") {
    OrderBook book("AAPL");
    Order o1{1, 1, "AAPL", Side::BUY, OrderType::LIMIT, 100.0, 5, 0};
    auto trades = book.addOrder(o1);
    REQUIRE(trades.empty());
    REQUIRE(book.bestBid() == 100.0);
    REQUIRE(book.bestAsk() == 0.0);
}

TEST_CASE("Crossing limit orders generate a trade", "[OrderBook]") {
    OrderBook book("AAPL");
    Order sell{1, 2, "AAPL", Side::SELL, OrderType::LIMIT, 101.0, 3, 0};
    book.addOrder(sell);

    Order buy{2, 1, "AAPL", Side::BUY, OrderType::LIMIT, 102.0, 2, 1};
    auto trades = book.addOrder(buy);

    REQUIRE(trades.size() == 1);
    REQUIRE(trades[0].quantity == 2);
    REQUIRE(trades[0].price    == 101.0);

    REQUIRE(book.bestAsk() == 101.0);
}

TEST_CASE("Market order sweeps multiple levels", "[OrderBook]") {
    OrderBook book("AAPL");
    book.addOrder({1,3,"AAPL",Side::SELL,OrderType::LIMIT,100.0,2,0});
    book.addOrder({2,4,"AAPL",Side::SELL,OrderType::LIMIT,102.0,5,1});

    auto trades = book.addOrder({3,5,"AAPL",Side::BUY,OrderType::MARKET,0.0,6,2});
    REQUIRE(trades.size() == 2);
    REQUIRE(trades[0].quantity == 2);
    REQUIRE(trades[0].price    == 100.0);
    REQUIRE(trades[1].quantity == 4);
    REQUIRE(trades[1].price    == 102.0);
}

TEST_CASE("Cancel resting order", "[OrderBook]") {
    OrderBook book("AAPL");
    book.addOrder({1,1,"AAPL",Side::BUY,OrderType::LIMIT,99.0,10,0});
    book.addOrder({1,0,"AAPL",Side::BUY,OrderType::CANCEL,0.0,0,1});
    REQUIRE(book.bestBid() == 0.0);
}
