#define CATCH_CONFIG_MAIN
#include "catch.hpp"
#include "../src/Order.h"

TEST_CASE("Order struct defaults", "[order]") {
    Order o{1, 42, "AAPL", Side::BUY, 150.0, 10, 0};
    REQUIRE(o.orderId == 1);
    REQUIRE(o.accountId == 42);
    REQUIRE(o.symbol == "AAPL");
    REQUIRE(o.side == Side::BUY);
    REQUIRE(o.price == 150.0);
    REQUIRE(o.quantity == 10);
}