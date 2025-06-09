#pragma once
#include <cstdint>
#include <string>

enum class Side   { BUY, SELL };
enum class OrderType { LIMIT, MARKET, CANCEL };

struct Order {
    uint64_t   orderId;
    uint64_t   accountId;
    std::string symbol;
    Side       side;
    OrderType  type;
    double     price;      // ignored for MARKET/CANCEL
    uint64_t   quantity;
    uint64_t   timestamp;  // ns since epoch
};