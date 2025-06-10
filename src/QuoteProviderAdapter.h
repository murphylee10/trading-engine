#pragma once
#include "Quote.h"
#include <concurrentqueue.h>

/// Base for any quote provider. start() runs a loop and pushes Quotes.
class QuoteProviderAdapter {
public:
  virtual ~QuoteProviderAdapter() = default;
  virtual void start(moodycamel::ConcurrentQueue<Quote>& quoteQueue) = 0;
};
