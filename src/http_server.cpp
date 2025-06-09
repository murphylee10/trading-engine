#include <boost/asio.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <nlohmann/json.hpp>
#include "http_server.h"
#include "MatchingEngine.h"

namespace asio  = boost::asio;
namespace beast = boost::beast;
namespace http  = beast::http;
using tcp       = asio::ip::tcp;
using json      = nlohmann::json;

// Handles a single HTTP request, using the passed-in engine
void handle_request(const http::request<http::string_body>& req,
                    std::shared_ptr<beast::tcp_stream>      stream,
                    MatchingEngine&                        engine)
{
    http::response<http::string_body> res{ http::status::ok, req.version() };
    res.set(http::field::content_type, "application/json");
    res.keep_alive(req.keep_alive());

    std::string target = std::string(req.target());
    if (target.rfind("/book/", 0) == 0) {
        auto pos = target.find('?');
        std::string sym = target.substr(6, (pos==std::string::npos ? std::string::npos : pos-6));
        size_t depth = 10;
        if (pos != std::string::npos) {
            auto q = target.substr(pos+1);
            if (q.rfind("depth=",0) == 0) depth = std::stoul(q.substr(6));
        }
        auto levels = engine.snapshotBook(sym, depth);
        json j; j["bids"] = json::array();
        for (auto& lvl : levels) {
            j["bids"].push_back({ {"price", lvl.price}, {"qty", lvl.quantity} });
        }
        res.body() = j.dump();

    } else if (target.rfind("/trades/", 0) == 0) {
        auto pos = target.find('?');
        std::string sym = target.substr(8, (pos==std::string::npos ? std::string::npos : pos-8));
        size_t limit = 10;
        if (pos != std::string::npos) {
            auto q = target.substr(pos+1);
            if (q.rfind("limit=",0) == 0) limit = std::stoul(q.substr(6));
        }
        auto recent = engine.recentTrades(sym, limit);
        json j = json::array();
        for (auto& t : recent) {
            j.push_back({
              {"tradeId",    t.tradeId},
              {"price",      t.price},
              {"qty",        t.quantity},
              {"buyOrderId", t.buyOrderId},
              {"sellOrderId",t.sellOrderId}
            });
        }
        res.body() = j.dump();

    } else {
        res.result(http::status::not_found);
        res.body() = R"({"error":"unknown endpoint"})";
    }

    res.prepare_payload();
    http::write(*stream, res);
}

void run_http_server(asio::io_context& ioc,
                     unsigned short    port,
                     MatchingEngine&   engine)
{
    tcp::acceptor acceptor{ioc, {tcp::v4(), port}};
    for (;;) {
        // Accept a new connection
        auto socket = acceptor.accept();
        // Wrap it in a Beast TCP stream for HTTP
        auto stream = std::make_shared<beast::tcp_stream>(std::move(socket));
        beast::flat_buffer buffer;
        http::request<http::string_body> req;

        // Read the HTTP request
        http::read(*stream, buffer, req);

        // Handle & respond
        handle_request(req, stream, engine);
    }
}
