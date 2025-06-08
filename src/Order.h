#pragma once
#include <cstdint>
#include <string>

enum class Side { BUY, SELL };

struct Order {
    uint64_t   orderId;
    uint64_t   accountId;
    std::string symbol;
    Side       side;
    double     price;       // ignored for market orders
    uint64_t   quantity;
    uint64_t   timestamp;   // nanoseconds since epoch
};