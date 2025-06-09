#pragma once
#include <map>
#include <deque>
#include <unordered_map>
#include <vector>
#include "Order.h"
#include "Trade.h"

class OrderBook {
public:
  explicit OrderBook(const std::string& symbol);

  std::vector<Trade> addOrder(const Order& o);

  void cancelOrder(uint64_t orderId);

  double bestBid() const;
  double bestAsk() const;

  // snapshot support
  struct Level {
    double    price;
    uint64_t  quantity;
  };
  std::vector<Level> getBids(size_t depth) const; // return up to `depth` best bids (highest price first)
  std::vector<Level> getAsks(size_t depth) const; // return up to `depth` best asks (lowest price first)

private:
  // price → queue of orders
  std::map<double, std::deque<Order>, std::greater<>> bids_;
  std::map<double, std::deque<Order>, std::less<>>    asks_;

  // quick lookup: orderId → (side, price)
  struct Loc { bool isBid; double price; };
  std::unordered_map<uint64_t, Loc> lookup_;

  std::string symbol_;

  // internal helpers
  std::vector<Trade> matchAtPrice(std::deque<Order>& sideQueue,
                                  double price,
                                  uint64_t incomingQty,
                                  uint64_t incomingOrderId,
                                  Side incomingSide);
};
