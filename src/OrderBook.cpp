#include "OrderBook.h"
#include <algorithm>
#include <chrono>

OrderBook::OrderBook(const std::string &symbol)
    : symbol_(symbol)
{
}

std::vector<Trade> OrderBook::addOrder(const Order &o)
{
  std::vector<Trade> trades;
  uint64_t remaining = o.quantity;

  if (o.type == OrderType::CANCEL)
  {
    cancelOrder(o.orderId);
    return trades;
  }

  double limitPrice = 0.0;
  if (o.type == OrderType::MARKET)
  {
    limitPrice = (o.side == Side::BUY
                      ? std::numeric_limits<double>::infinity()
                      : -std::numeric_limits<double>::infinity());
  }
  else
  {
    limitPrice = o.price;
  }

  if (o.side == Side::BUY)
  {
    auto priceCheck = [&](double lp, double lvl)
    { return lp >= lvl; };
    while (remaining > 0 && !asks_.empty())
    {
      auto itLevel = asks_.begin();
      double lvlPrice = itLevel->first;
      if (!priceCheck(limitPrice, lvlPrice))
        break;

      auto &queue = itLevel->second;
      auto newTrades = matchAtPrice(queue, lvlPrice, remaining, o.orderId, o.side);
      trades.insert(trades.end(), newTrades.begin(), newTrades.end());
      for (auto &t : newTrades)
        remaining -= t.quantity;
      if (queue.empty())
        asks_.erase(itLevel);
    }
    if (o.type == OrderType::LIMIT && remaining > 0)
    {
      Order rest = o;
      rest.quantity = remaining;
      bids_[o.price].push_back(rest);
      lookup_[o.orderId] = {true, o.price};
    }
  }
  else
  {
    auto priceCheck = [&](double lp, double lvl)
    { return lp <= lvl; };
    while (remaining > 0 && !bids_.empty())
    {
      auto itLevel = bids_.begin();
      double lvlPrice = itLevel->first;
      if (!priceCheck(limitPrice, lvlPrice))
        break;

      auto &queue = itLevel->second;
      auto newTrades = matchAtPrice(queue, lvlPrice, remaining, o.orderId, o.side);
      trades.insert(trades.end(), newTrades.begin(), newTrades.end());
      for (auto &t : newTrades)
        remaining -= t.quantity;
      if (queue.empty())
        bids_.erase(itLevel);
    }
    if (o.type == OrderType::LIMIT && remaining > 0)
    {
      Order rest = o;
      rest.quantity = remaining;
      asks_[o.price].push_back(rest);
      lookup_[o.orderId] = {false, o.price};
    }
  }

  return trades;
}

void OrderBook::cancelOrder(uint64_t orderId)
{
  auto it = lookup_.find(orderId);
  if (it == lookup_.end())
    return;

  bool isBid = it->second.isBid;
  double price = it->second.price;
  auto &queue = (isBid ? bids_[price] : asks_[price]);

  queue.erase(std::remove_if(queue.begin(), queue.end(),
                             [&](const Order &o)
                             { return o.orderId == orderId; }),
              queue.end());

  if (queue.empty())
  {
    if (isBid)
      bids_.erase(price);
    else
      asks_.erase(price);
  }

  lookup_.erase(it);
}

double OrderBook::bestBid() const
{
  return bids_.empty() ? 0.0 : bids_.begin()->first;
}

double OrderBook::bestAsk() const
{
  return asks_.empty() ? 0.0 : asks_.begin()->first;
}

std::vector<Trade> OrderBook::matchAtPrice(std::deque<Order> &sideQueue,
                                           double price,
                                           uint64_t incomingQty,
                                           uint64_t incomingOrderId,
                                           Side incomingSide)
{
  std::vector<Trade> trades;
  uint64_t remaining = incomingQty;

  while (remaining > 0 && !sideQueue.empty())
  {
    Order resting = sideQueue.front();
    uint64_t tradeQty = std::min(remaining, resting.quantity);

    Trade t;
    t.tradeId = 0;
    if (incomingSide == Side::BUY)
    {
      t.buyOrderId = incomingOrderId;
      t.sellOrderId = resting.orderId;
    }
    else
    {
      t.buyOrderId = resting.orderId;
      t.sellOrderId = incomingOrderId;
    }
    t.symbol = symbol_;
    t.price = price;
    t.quantity = tradeQty;
    t.timestamp = std::chrono::duration_cast<std::chrono::nanoseconds>(
                      std::chrono::high_resolution_clock::now().time_since_epoch())
                      .count();

    trades.push_back(t);

    if (resting.quantity > tradeQty)
    {
      sideQueue.front().quantity -= tradeQty;
      remaining = 0;
    }
    else
    {
      sideQueue.pop_front();
      remaining -= tradeQty;
    }
  }

  return trades;
}

std::vector<OrderBook::Level> OrderBook::getBids(size_t depth) const
{
  std::vector<Level> levels;
  levels.reserve(depth);
  for (auto it = bids_.begin(); it != bids_.end() && levels.size() < depth; ++it)
  {
    uint64_t totalQty = 0;
    for (auto &o : it->second)
      totalQty += o.quantity;
    levels.push_back({it->first, totalQty});
  }
  return levels;
}

std::vector<OrderBook::Level> OrderBook::getAsks(size_t depth) const
{
  std::vector<Level> levels;
  levels.reserve(depth);
  for (auto it = asks_.begin(); it != asks_.end() && levels.size() < depth; ++it)
  {
    uint64_t totalQty = 0;
    for (auto &o : it->second)
      totalQty += o.quantity;
    levels.push_back({it->first, totalQty});
  }
  return levels;
}
