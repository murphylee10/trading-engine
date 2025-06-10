#include <iostream>
#include <string>
#include <thread>

#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>
#include <boost/beast.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/beast/ssl.hpp>
#include <nlohmann/json.hpp>

#include "BinanceWebSocketAdapter.h"

using json   = nlohmann::json;
namespace asio  = boost::asio;
namespace ssl   = asio::ssl;
namespace beast = boost::beast;
namespace ws    = beast::websocket;
using tcp       = asio::ip::tcp;

BinanceWebSocketAdapter::BinanceWebSocketAdapter(
    std::vector<std::string> symbols)
  : symbols_(std::move(symbols))
{}

void BinanceWebSocketAdapter::start(
    moodycamel::ConcurrentQueue<Quote>& quoteQueue)
{
    try {
        asio::io_context ioc;
        ssl::context ctx{ssl::context::tlsv12_client};
        // Insecure for demo; in prod load real root CAs
        ctx.set_verify_mode(ssl::verify_none);

        // Resolve DNS
        tcp::resolver resolver{ioc};
        auto const results = resolver.resolve("stream.binance.com", "9443");

        // Create the underlying SSL stream
        beast::ssl_stream<tcp::socket> ssl_stream{ioc, ctx};
        asio::connect(ssl_stream.next_layer(), results.begin(), results.end());
        ssl_stream.handshake(ssl::stream_base::client);

        // Upgrade to WebSocket over TLS
        ws::stream<beast::ssl_stream<tcp::socket>> ws{std::move(ssl_stream)};

        // Subscribe to trade streams
        std::string path = "/ws/";
        for (size_t i = 0; i < symbols_.size(); ++i) {
            if (i) path += '/';
            path += symbols_[i] + "@trade";
        }
        ws.handshake("stream.binance.com", path);

        beast::flat_buffer buffer;
        while (true) {
            ws.read(buffer);
            auto msg = beast::buffers_to_string(buffer.data());
            buffer.consume(buffer.size());

            // Parse JSON trade event
            auto j = json::parse(msg);
            Quote q;
            q.symbol    = j["s"].get<std::string>();
            q.price     = std::stod(j["p"].get<std::string>());
            q.timestamp = j["T"].get<uint64_t>() * 1'000'000;  // ms → ns

            quoteQueue.enqueue(q);
        }
    } catch (const std::exception& e) {
        std::cerr << "WebSocket adapter error: " << e.what() << "\n";
    }
}
