#include "catch.hpp"
#include "../src/MatchingEngine.h"

TEST_CASE("Engine routes to OrderBook", "[MatchingEngine]") {
    MatchingEngine eng;

    Order sell{1,2,"TSLA",Side::SELL,OrderType::LIMIT,200.0,3,0};
    auto t0 = eng.onNewOrder(sell);
    REQUIRE(t0.empty());

    Order buy{2,1,"TSLA",Side::BUY,OrderType::LIMIT,205.0,2,1};
    auto t1 = eng.onNewOrder(buy);
    REQUIRE(t1.size() == 1);
    REQUIRE(t1[0].tradeId == 1);
    REQUIRE(t1[0].price   == 200.0);
}
