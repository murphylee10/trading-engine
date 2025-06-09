#include "MatchingEngine.h"

std::vector<Trade> MatchingEngine::onNewOrder(const Order& order) {
    auto it = books_.find(order.symbol);
    if (it == books_.end()) {
        // emplace a new OrderBook constructed with the symbol
        auto res = books_.emplace(
            order.symbol,
            OrderBook(order.symbol)
        );
        it = res.first;
    }
    OrderBook& book = it->second;

    // Let the book generate trades
    auto trades = book.addOrder(order);

    // Assign unique trade IDs and buffer
    for (auto& t : trades) {
        t.tradeId = nextTradeId_++;
        trades_.push_back(t);
    }
    return trades;
}

void MatchingEngine::onCancel(uint64_t orderId, const std::string& symbol) {
    auto it = books_.find(symbol);
    if (it != books_.end()) {
        it->second.cancelOrder(orderId);
    }
}

std::vector<Trade> MatchingEngine::collectTrades() {
    auto out = std::move(trades_);
    trades_.clear();
    return out;
}

std::vector<OrderBook::Level>
MatchingEngine::snapshotBook(const std::string& symbol, size_t depth) {
    auto it = books_.find(symbol);
    if (it == books_.end()) return {};
    return it->second.getBids(depth);
}

std::vector<Trade>
MatchingEngine::recentTrades(const std::string& symbol, size_t limit) {
    std::vector<Trade> out;
    // scan `trades_` from back, pick up to `limit` matching symbol
    for (auto it = trades_.rbegin();
         it != trades_.rend() && out.size() < limit;
         ++it) {
      if (it->symbol == symbol) out.push_back(*it);
    }
    // reverse so oldest→newest
    std::reverse(out.begin(), out.end());
    return out;
}