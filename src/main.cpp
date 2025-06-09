#include <iostream>
#include <thread>
#include <fstream>
#include <chrono>
#include <sstream>

#include <boost/asio.hpp>
#include <rdkafkacpp.h>
#include <concurrentqueue.h>
#include <nlohmann/json.hpp>

#include "http_server.h"
#include "Order.h"
#include "MatchingEngine.h"

using json = nlohmann::json;
using boost::asio::ip::tcp;

void engineLoop(moodycamel::ConcurrentQueue<Order>& inQ,
                MatchingEngine& engine,
                std::ofstream& orderLog,
                std::ofstream& tradeLog,
                RdKafka::Producer* producer,
                RdKafka::Topic*    topicOrders,
                RdKafka::Topic*    topicTrades)
{
    Order o;
    while (true) {
        if (inQ.try_dequeue(o)) {
            // Local log
            orderLog << o.orderId << ','
                     << static_cast<int>(o.type) << ','
                     << static_cast<int>(o.side) << ','
                     << o.price << ','
                     << o.quantity << '\n';
            orderLog.flush();

            // Produce order → Kafka topicOrders
            {
                json jo = {
                    {"orderId",   o.orderId},
                    {"accountId", o.accountId},
                    {"symbol",    o.symbol},
                    {"side",      static_cast<int>(o.side)},
                    {"type",      static_cast<int>(o.type)},
                    {"price",     o.price},
                    {"quantity",  o.quantity},
                    {"timestamp", o.timestamp}
                };
                auto payload = jo.dump();
                producer->produce(
                    /*topic*/      topicOrders,
                    /*partition*/  RdKafka::Topic::PARTITION_UA,
                    /*flags*/      RdKafka::Producer::RK_MSG_COPY,
                    /*payload*/    const_cast<char*>(payload.data()),
                    /*len*/        payload.size(),
                    /*keyptr*/     nullptr,
                    /*keylen*/     0,
                    /*opaque*/     nullptr
                );
                producer->poll(0);
            }

            // Matching
            auto trades = engine.onNewOrder(o);
            for (auto& t : trades) {
                // Local log
                tradeLog << t.tradeId    << ','
                         << t.buyOrderId << ','
                         << t.sellOrderId<< ','
                         << t.price      << ','
                         << t.quantity   << '\n';
                tradeLog.flush();

                // Produce trade → Kafka topicTrades
                json jt = {
                    {"tradeId",    t.tradeId},
                    {"buyOrderId", t.buyOrderId},
                    {"sellOrderId",t.sellOrderId},
                    {"symbol",     t.symbol},
                    {"price",      t.price},
                    {"quantity",   t.quantity},
                    {"timestamp",  t.timestamp}
                };
                auto payload = jt.dump();
                producer->produce(
                    /*topic*/      topicTrades,
                    /*partition*/  RdKafka::Topic::PARTITION_UA,
                    /*flags*/      RdKafka::Producer::RK_MSG_COPY,
                    /*payload*/    const_cast<char*>(payload.data()),
                    /*len*/        payload.size(),
                    /*keyptr*/     nullptr,
                    /*keylen*/     0,
                    /*opaque*/     nullptr
                );
                producer->poll(0);
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

    //
    // Kafka producer setup
    //
    std::string brokers = "localhost:9092";
    std::string errstr;
    RdKafka::Conf* conf = RdKafka::Conf::create(RdKafka::Conf::CONF_GLOBAL);
    conf->set("metadata.broker.list", brokers, errstr);

    RdKafka::Producer* producer = RdKafka::Producer::create(conf, errstr);
    if (!producer) {
        std::cerr << "Failed to create Kafka producer: " << errstr << "\n";
        return 1;
    }
    delete conf;

    // Create Topic handles once
    RdKafka::Topic* topicOrders =
        RdKafka::Topic::create(producer, "orders",  nullptr, errstr);
    RdKafka::Topic* topicTrades =
        RdKafka::Topic::create(producer, "trades",  nullptr, errstr);

    std::thread engThread(engineLoop,
                          std::ref(inQ),
                          std::ref(engine),
                          std::ref(orderLog),
                          std::ref(tradeLog),
                          producer,
                          topicOrders,
                          topicTrades);

    std::thread httpThread([&](){
        boost::asio::io_context httpIo{1};
        run_http_server(httpIo, 8080, engine);
    });
    httpThread.detach();

    // TCP accept loop for orders
    boost::asio::io_context io_ctx;
    tcp::acceptor acceptor(io_ctx, tcp::endpoint(tcp::v4(), 9000));
    std::cout << "Trading engine listening on port 9000\n";

    for (;;) {
        tcp::socket socket(io_ctx);
        acceptor.accept(socket);
        std::thread([sock = std::move(socket), &inQ]() mutable {
            boost::asio::streambuf buf;
            std::string line;
            while (true) {
                boost::system::error_code ec;
                auto n = boost::asio::read_until(sock, buf, '\n', ec);
                if (ec) break;
                std::istream is(&buf);
                std::getline(is, line);
                buf.consume(n);
                if (line.empty()) continue;

                Order o;
                std::istringstream ss(line);
                std::string token;
                std::getline(ss, token, ','); o.orderId   = std::stoull(token);
                std::getline(ss, token, ','); o.accountId = std::stoull(token);
                std::getline(ss, o.symbol, ',');
                std::getline(ss, token, ','); o.side      = static_cast<Side>(std::stoi(token));
                std::getline(ss, token, ','); o.type      = static_cast<OrderType>(std::stoi(token));
                std::getline(ss, token, ','); o.price     = std::stod(token);
                std::getline(ss, token, ','); o.quantity  = std::stoull(token);
                std::getline(ss, token);      o.timestamp = std::stoull(token);

                inQ.enqueue(o);
            }
        }).detach();
    }

    engThread.join();
    return 0;
}
