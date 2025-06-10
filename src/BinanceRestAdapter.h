#pragma once
#include "QuoteProviderAdapter.h"
#include "httplib.h"
#include <vector>
#include <string>

class BinanceRestAdapter : public QuoteProviderAdapter {
public:
  /// symbols: e.g. {"BTCUSDT","ETHUSDT"}, intervalMs: poll interval
  BinanceRestAdapter(std::vector<std::string> symbols, uint64_t intervalMs = 500);
  void start(moodycamel::ConcurrentQueue<Quote>& quoteQueue) override;

private:
  std::vector<std::string> symbols_;
  uint64_t                 intervalMs_;
  httplib::SSLClient       cli_;
};
