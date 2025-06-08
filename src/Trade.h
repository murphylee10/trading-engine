#pragma once
#include <cstdint>
#include <string>

struct Trade {
    uint64_t   tradeId;
    uint64_t   buyOrderId;
    uint64_t   sellOrderId;
    std::string symbol;
    double     price;
    uint64_t   quantity;
    uint64_t   timestamp;
};