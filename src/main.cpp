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
using tcp  = boost::asio::ip::tcp;
namespace chrono = std::chrono;

// Helper to serialize and produce a JSON object to Kafka
void produceJson(RdKafka::Producer* producer,
                 RdKafka::Topic*    topic,
                 const json&        j)
{
    auto payload = j.dump();
    producer->produce(
        topic,
        RdKafka::Topic::PARTITION_UA,
        RdKafka::Producer::RK_MSG_COPY,
        const_cast<char*>(payload.data()),
        payload.size(),
        nullptr, 0, nullptr
    );
    producer->poll(0);
}

// Engine thread: dequeue orders, log, match, log trades, produce to Kafka topics
void engineLoop(moodycamel::ConcurrentQueue<Order>& inQ,
                MatchingEngine&                     engine,
                std::ofstream&                      orderLog,
                std::ofstream&                      tradeLog,
                RdKafka::Producer*                  producer,
                RdKafka::Topic*                     topicOrders,
                RdKafka::Topic*                     topicTrades,
                RdKafka::Topic*                     topicMetrics)
{
    Order o;
    size_t orderCount = 0;
    auto windowStart = chrono::high_resolution_clock::now();

    while (true) {
        if (!inQ.try_dequeue(o)) {
            std::this_thread::sleep_for(chrono::milliseconds(1));
            continue;
        }

        // 1) Local order log
        orderLog << o.orderId << ','
                 << static_cast<int>(o.type) << ','
                 << static_cast<int>(o.side) << ','
                 << o.price << ','
                 << o.quantity << '\n';
        orderLog.flush();

        // 2) Produce order to Kafka
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
            produceJson(producer, topicOrders, jo);
        }

        // 3) Measure latency and emit metric
        auto t0 = chrono::high_resolution_clock::now();
        auto trades = engine.onNewOrder(o);
        auto t1 = chrono::high_resolution_clock::now();
        auto latency_ns = chrono::duration_cast<chrono::nanoseconds>(t1 - t0).count();

        {
            json m = {
                {"metric",    "order_latency_ns"},
                {"value",     latency_ns},
                {"symbol",    o.symbol},
                {"timestamp", static_cast<int64_t>(
                    chrono::duration_cast<chrono::nanoseconds>(
                        t1.time_since_epoch()).count())}
            };
            produceJson(producer, topicMetrics, m);
        }

        // 4) Count orders/sec
        orderCount++;
        auto now = chrono::high_resolution_clock::now();
        if (now - windowStart >= chrono::seconds(1)) {
            json m = {
                {"metric",    "orders_per_sec"},
                {"value",     static_cast<int>(orderCount)},
                {"timestamp", static_cast<int64_t>(
                    chrono::duration_cast<chrono::nanoseconds>(
                        now.time_since_epoch()).count())}
            };
            produceJson(producer, topicMetrics, m);
            orderCount   = 0;
            windowStart  = now;
        }

        // 5) Log & produce trades
        for (auto& t : trades) {
            tradeLog << t.tradeId    << ','
                     << t.buyOrderId << ','
                     << t.sellOrderId<< ','
                     << t.price      << ','
                     << t.quantity   << '\n';
            tradeLog.flush();

            json jt = {
                {"tradeId",    t.tradeId},
                {"buyOrderId", t.buyOrderId},
                {"sellOrderId",t.sellOrderId},
                {"symbol",     t.symbol},
                {"price",      t.price},
                {"quantity",   t.quantity},
                {"timestamp",  t.timestamp}
            };
            produceJson(producer, topicTrades, jt);
        }
    }
}

int main() {
    // In-memory handoff queue and matching engine
    moodycamel::ConcurrentQueue<Order> inQ;
    MatchingEngine engine;

    // Local logs
    std::ofstream orderLog("orders.log", std::ios::app);
    std::ofstream tradeLog("trades.log", std::ios::app);

    // --- Kafka producer setup ---
    std::string brokers = "localhost:9092";
    std::string errstr;
    auto* conf = RdKafka::Conf::create(RdKafka::Conf::CONF_GLOBAL);
    conf->set("metadata.broker.list", brokers, errstr);

    auto* producer = RdKafka::Producer::create(conf, errstr);
    if (!producer) {
        std::cerr << "Kafka producer creation failed: " << errstr << "\n";
        return 1;
    }
    delete conf;

    // Topics
    auto* topicOrders  = RdKafka::Topic::create(producer, "orders",  nullptr, errstr);
    auto* topicTrades  = RdKafka::Topic::create(producer, "trades",  nullptr, errstr);
    auto* topicMetrics = RdKafka::Topic::create(producer, "metrics", nullptr, errstr);

    // --- Launch engine thread ---
    std::thread engThread(engineLoop,
        std::ref(inQ), std::ref(engine),
        std::ref(orderLog), std::ref(tradeLog),
        producer, topicOrders, topicTrades, topicMetrics
    );

    // --- Launch HTTP server ---
    std::thread httpThread([&](){
        boost::asio::io_context httpIo{1};
        run_http_server(httpIo, 8080, engine);
    });
    httpThread.detach();

    // --- TCP accept loop for orders ---
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
                std::getline(ss, token, ','); o.side      = static_cast<Side>(    std::stoi(token));
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
