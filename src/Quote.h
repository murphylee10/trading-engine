#pragma once
#include <string>
#include <cstdint>

struct Quote {
    std::string symbol;
    double      price;
    uint64_t    timestamp;  // ns since epoch
};
