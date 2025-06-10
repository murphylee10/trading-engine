#pragma once

#include <boost/asio.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>
#include <concurrentqueue.h>
#include "QuoteProviderAdapter.h"

namespace asio  = boost::asio;
namespace beast = boost::beast;
namespace ws    = beast::websocket;
using tcp       = asio::ip::tcp;

class BinanceWebSocketAdapter : public QuoteProviderAdapter {
public:
    BinanceWebSocketAdapter(std::vector<std::string> symbols);
    void start(moodycamel::ConcurrentQueue<Quote>& quoteQueue) override;

private:
    std::vector<std::string> symbols_;
};
