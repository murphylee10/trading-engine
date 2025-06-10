#include "BinanceRestAdapter.h"
#include <chrono>
#include <thread>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

BinanceRestAdapter::BinanceRestAdapter(std::vector<std::string> symbols,
                                       uint64_t                 intervalMs)
  : symbols_(std::move(symbols))
  , intervalMs_(intervalMs)
  , cli_("api.binance.com", 443)   // HTTPS client
{
    cli_.enable_server_certificate_verification(false);
}

void BinanceRestAdapter::start(
    moodycamel::ConcurrentQueue<Quote>& quoteQueue)
{
    while (true) {
        for (auto& sym : symbols_) {
            auto res = cli_.Get(("/api/v3/ticker/price?symbol=" + sym).c_str());
            if (res && res->status == 200) {
                auto j = json::parse(res->body);
                Quote q;
                q.symbol = j["symbol"].get<std::string>();
                q.price  = std::stod(j["price"].get<std::string>());
                q.timestamp = std::chrono::duration_cast<std::chrono::nanoseconds>(
                    std::chrono::high_resolution_clock::now()
                        .time_since_epoch()).count();
                quoteQueue.enqueue(q);
            }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(intervalMs_));
    }
}
