#include <iostream>
#include <thread>
#include <fstream>
#include <chrono>
#include <sstream>

#include <boost/asio.hpp>
#include <concurrentqueue.h>
#include <nlohmann/json.hpp>
#include "http_server.h" 

#include "Order.h"
#include "MatchingEngine.h"


using boost::asio::ip::tcp;

// engine thread: dequeue orders, log them, process, log trades
void engineLoop(moodycamel::ConcurrentQueue<Order>& inQ,
                MatchingEngine& engine,
                std::ofstream& orderLog,
                std::ofstream& tradeLog)
{
    Order o;
    while (true) {
        if (inQ.try_dequeue(o)) {
            orderLog 
                << o.orderId << ',' 
                << static_cast<int>(o.type) << ',' 
                << static_cast<int>(o.side) << ',' 
                << o.price       << ',' 
                << o.quantity    << '\n';
            orderLog.flush();

            auto trades = engine.onNewOrder(o);
            for (auto& t : trades) {
                tradeLog 
                    << t.tradeId    << ',' 
                    << t.buyOrderId << ',' 
                    << t.sellOrderId<< ',' 
                    << t.price      << ',' 
                    << t.quantity   << '\n';
                tradeLog.flush();
            }
        } else {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    }
}

int main() {
    moodycamel::ConcurrentQueue<Order> inQ;
    MatchingEngine engine;

    std::ofstream orderLog("orders.log", std::ios::app);
    std::ofstream tradeLog("trades.log", std::ios::app);

    std::thread engThread(engineLoop,
                          std::ref(inQ),
                          std::ref(engine),
                          std::ref(orderLog),
                          std::ref(tradeLog));

    std::thread httpThread([&](){
        asio::io_context httpIo{1};
        run_http_server(httpIo, 8080, engine);
    });
    httpThread.detach();

    boost::asio::io_context io_ctx;
    tcp::acceptor acceptor(io_ctx, {tcp::v4(), 9000});
    std::cout << "Trading engine listening on port 9000\n";

    // accept loop — spawn a thread per client
    for (;;) {
        tcp::socket socket(io_ctx);
        acceptor.accept(socket);
        std::cout << "Client connected: " 
                  << socket.remote_endpoint() << '\n';

        std::thread([sock = std::move(socket), &inQ]() mutable {
            boost::asio::streambuf buf;
            std::string line;

            while (true) {
                boost::system::error_code ec;
                std::size_t n = boost::asio::read_until(sock, buf, '\n', ec);
                if (ec) {
                    std::cerr << "Session error: " << ec.message() << "\n";
                    break;
                }

                std::istream is(&buf);
                std::getline(is, line);
                if (line.empty()) {
                    buf.consume(n);
                    continue;
                }

                std::istringstream ss(line);
                std::string token;
                Order o;

                // orderId
                std::getline(ss, token, ',');
                o.orderId = std::stoull(token);

                // accountId
                std::getline(ss, token, ',');
                o.accountId = std::stoull(token);

                // symbol
                std::getline(ss, o.symbol, ',');

                // side
                std::getline(ss, token, ',');
                o.side = static_cast<Side>(std::stoi(token));

                // type
                std::getline(ss, token, ',');
                o.type = static_cast<OrderType>(std::stoi(token));

                // price
                std::getline(ss, token, ',');
                o.price = std::stod(token);

                // quantity
                std::getline(ss, token, ',');
                o.quantity = std::stoull(token);

                // timestamp (rest of line)
                std::getline(ss, token);
                o.timestamp = std::stoull(token);

                // enqueue for matching
                inQ.enqueue(o);

                // remove consumed bytes
                buf.consume(n);
            }
        }).detach();
    }

    engThread.join();

    return 0;
}
