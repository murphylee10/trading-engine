#include <boost/asio.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <nlohmann/json.hpp>
#include "http_server.h"

namespace asio  = boost::asio;
namespace beast = boost::beast;
namespace http  = beast::http;
using tcp       = asio::ip::tcp;
using json      = nlohmann::json;

// Handles a single HTTP request, with CORS
static void handle_request(
    const http::request<http::string_body>& req,
    std::shared_ptr<beast::tcp_stream>      stream,
    MatchingEngine&                         engine)
{
    // Convert target to std::string
    std::string target(req.target().data(), req.target().size());

    // If this is a CORS preflight request, just reply with the allowed headers
    if (req.method() == http::verb::options) {
        http::response<http::empty_body> res{http::status::no_content, req.version()};
        res.set(http::field::access_control_allow_origin,  "*");
        res.set(http::field::access_control_allow_methods, "GET,OPTIONS");
        res.set(http::field::access_control_allow_headers, "Content-Type");
        res.keep_alive(req.keep_alive());
        http::write(*stream, res);
        return;
    }

    // Prepare a reusable "string_body" response
    http::response<http::string_body> res{http::status::ok, req.version()};
    res.set(http::field::content_type, "application/json");
    // CORS header on every response
    res.set(http::field::access_control_allow_origin, "*");
    res.keep_alive(req.keep_alive());

    // — GET /book/{symbol}?depth={n}
    if (req.method() == http::verb::get && target.rfind("/book/", 0) == 0) {
        // parse symbol and optional ?depth=
        auto pos = target.find('?');
        std::string sym = target.substr(6,
            pos == std::string::npos ? std::string::npos : pos - 6);
        size_t depth = 10;
        if (pos != std::string::npos &&
            target.substr(pos).rfind("?depth=", 0) == 0)
        {
            depth = std::stoul(target.substr(pos + 7));
        }

        auto levels = engine.snapshotBook(sym, depth);
        json j; j["bids"] = json::array();
        for (auto& lvl : levels)
            j["bids"].push_back({{"price", lvl.price}, {"qty", lvl.quantity}});

        res.body() = j.dump();
        res.prepare_payload();
        http::write(*stream, res);
        return;
    }

    // — GET /trades/{symbol}?limit={n}
    if (req.method() == http::verb::get && target.rfind("/trades/", 0) == 0) {
        auto pos = target.find('?');
        std::string sym = target.substr(8,
            pos == std::string::npos ? std::string::npos : pos - 8);
        size_t limit = 10;
        if (pos != std::string::npos &&
            target.substr(pos).rfind("?limit=", 0) == 0)
        {
            limit = std::stoul(target.substr(pos + 7));
        }

        auto recent = engine.recentTrades(sym, limit);
        json j = json::array();
        for (auto& t : recent) {
            j.push_back({
              {"tradeId",     t.tradeId},
              {"price",       t.price},
              {"qty",         t.quantity},
              {"buyOrderId",  t.buyOrderId},
              {"sellOrderId", t.sellOrderId}
            });
        }

        res.body() = j.dump();
        res.prepare_payload();
        http::write(*stream, res);
        return;
    }

    // — Fallback 404
    res.result(http::status::not_found);
    res.body() = R"({"error":"unknown endpoint"})";
    res.prepare_payload();
    http::write(*stream, res);
}

void run_http_server(asio::io_context&   ioc,
                     unsigned short      port,
                     MatchingEngine&     engine)
{
    tcp::acceptor acceptor{ioc, {tcp::v4(), port}};
    for (;;) {
        auto socket = acceptor.accept();
        auto stream = std::make_shared<beast::tcp_stream>(std::move(socket));
        beast::flat_buffer buffer;
        http::request<http::string_body> req;
        http::read(*stream, buffer, req);
        handle_request(req, stream, engine);
    }
}
