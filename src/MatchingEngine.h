#pragma once
#include <unordered_map>
#include <vector>
#include "OrderBook.h"
#include "Order.h"
#include "Trade.h"

class MatchingEngine {
public:
  std::vector<Trade> onNewOrder(const Order& order);
  void onCancel(uint64_t orderId, const std::string& symbol);
  std::vector<Trade> collectTrades();

  std::vector<OrderBook::Level>
  snapshotBook(const std::string& symbol, size_t depth);

  std::vector<Trade>
  recentTrades(const std::string& symbol, size_t limit);

private:
  std::unordered_map<std::string, OrderBook> books_;
  std::vector<Trade> trades_;
  uint64_t nextTradeId_ = 1;
};
