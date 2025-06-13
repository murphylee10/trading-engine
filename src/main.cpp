#include <iostream>
#include <thread>
#include <fstream>
#include <chrono>
#include <sstream>

#include <boost/asio.hpp>
#include <rdkafkacpp.h>
#include <concurrentqueue.h>
#include <nlohmann/json.hpp>

#include "Order.h"
#include "MatchingEngine.h"
#include "http_server.h"

using json = nlohmann::json;
using tcp = boost::asio::ip::tcp;
namespace chrono = std::chrono;

// ----------------------------------------------------------------------------
// Helper: serialize a json object and publish to a Kafka topic
// ----------------------------------------------------------------------------
void produceJson(RdKafka::Producer *producer,
                 RdKafka::Topic *topic,
                 const json &j)
{
  auto payload = j.dump();
  producer->produce(
      topic,
      RdKafka::Topic::PARTITION_UA,
      RdKafka::Producer::RK_MSG_COPY,
      const_cast<char *>(payload.data()),
      payload.size(),
      nullptr, 0, nullptr);
  producer->poll(0);
}

// ----------------------------------------------------------------------------
// Engine thread: consume orders, log, match, log trades, emit Kafka metrics
// ----------------------------------------------------------------------------
void engineLoop(moodycamel::ConcurrentQueue<Order> &inQ,
                MatchingEngine &engine,
                std::ofstream &orderLog,
                std::ofstream &tradeLog,
                RdKafka::Producer *producer,
                RdKafka::Topic *topicOrders,
                RdKafka::Topic *topicTrades,
                RdKafka::Topic *topicMetrics)
{
  Order o;
  size_t orderCount = 0;
  auto windowStart = chrono::high_resolution_clock::now();

  while (true)
  {
    if (!inQ.try_dequeue(o))
    {
      std::this_thread::sleep_for(chrono::milliseconds(1));
      continue;
    }

    orderLog
        << o.orderId << ','
        << static_cast<int>(o.type) << ','
        << static_cast<int>(o.side) << ','
        << o.price << ','
        << o.quantity << '\n';
    orderLog.flush();

    {
      json jo = {
          {"orderId", o.orderId},
          {"accountId", o.accountId},
          {"symbol", o.symbol},
          {"side", static_cast<int>(o.side)},
          {"type", static_cast<int>(o.type)},
          {"price", o.price},
          {"quantity", o.quantity},
          {"timestamp", o.timestamp}};
      produceJson(producer, topicOrders, jo);
    }

    auto t0 = chrono::high_resolution_clock::now();
    auto trades = engine.onNewOrder(o);
    auto t1 = chrono::high_resolution_clock::now();
    auto latency_ns = chrono::duration_cast<chrono::nanoseconds>(t1 - t0).count();

    {
      json m = {
          {"metric", "order_latency_ns"},
          {"value", latency_ns},
          {"symbol", o.symbol},
          {"timestamp", static_cast<int64_t>(
                            chrono::duration_cast<chrono::nanoseconds>(
                                t1.time_since_epoch())
                                .count())}};
      produceJson(producer, topicMetrics, m);
    }

    orderCount++;
    auto now = chrono::high_resolution_clock::now();
    if (now - windowStart >= chrono::seconds(1))
    {
      json m = {
          {"metric", "orders_per_sec"},
          {"value", static_cast<int>(orderCount)},
          {"timestamp", static_cast<int64_t>(
                            chrono::duration_cast<chrono::nanoseconds>(
                                now.time_since_epoch())
                                .count())}};
      produceJson(producer, topicMetrics, m);
      orderCount = 0;
      windowStart = now;
    }

    for (auto &t : trades)
    {
      tradeLog
          << t.tradeId << ','
          << t.buyOrderId << ','
          << t.sellOrderId << ','
          << t.price << ','
          << t.quantity << '\n';
      tradeLog.flush();

      json jt = {
          {"tradeId", t.tradeId},
          {"buyOrderId", t.buyOrderId},
          {"sellOrderId", t.sellOrderId},
          {"symbol", t.symbol},
          {"price", t.price},
          {"quantity", t.quantity},
          {"timestamp", t.timestamp}};
      produceJson(producer, topicTrades, jt);
    }
  }
}

// ----------------------------------------------------------------------------
// main(): setup Kafka, engine, HTTP, and TCP ingest
// ----------------------------------------------------------------------------
int main()
{
  moodycamel::ConcurrentQueue<Order> inQ;
  MatchingEngine engine;
  std::ofstream orderLog("orders.log", std::ios::app);
  std::ofstream tradeLog("trades.log", std::ios::app);

  std::string brokers = "localhost:9092";
  std::string errstr;
  auto *conf = RdKafka::Conf::create(RdKafka::Conf::CONF_GLOBAL);
  conf->set("metadata.broker.list", brokers, errstr);

  auto *producer = RdKafka::Producer::create(conf, errstr);
  if (!producer)
  {
    std::cerr << "Kafka producer error: " << errstr << "\n";
    return 1;
  }
  delete conf;

  auto *topicOrders = RdKafka::Topic::create(producer, "orders", nullptr, errstr);
  auto *topicTrades = RdKafka::Topic::create(producer, "trades", nullptr, errstr);
  auto *topicMetrics = RdKafka::Topic::create(producer, "metrics", nullptr, errstr);

  std::thread engThread(engineLoop,
                        std::ref(inQ), std::ref(engine),
                        std::ref(orderLog), std::ref(tradeLog),
                        producer, topicOrders, topicTrades, topicMetrics);

  std::thread httpThread([&]()
                         {
        boost::asio::io_context ioc{1};
        run_http_server(ioc, 8080, engine); });
  httpThread.detach();

  boost::asio::io_context io_ctx{1};
  tcp::acceptor acceptor(io_ctx, {tcp::v4(), 9000});
  std::cout << "Matching engine listening on port 9000\n";

  for (;;)
  {
    tcp::socket socket(io_ctx);
    acceptor.accept(socket);

    std::thread([sock = std::move(socket), &inQ]() mutable
                {
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

                std::istringstream ss(line);
                Order o; std::string tok;
                std::getline(ss, tok, ','); o.orderId   = std::stoull(tok);
                std::getline(ss, tok, ','); o.accountId = std::stoull(tok);
                std::getline(ss, o.symbol,   ',');
                std::getline(ss, tok,       ','); o.side     = static_cast<Side>(std::stoi(tok));
                std::getline(ss, tok,       ','); o.type     = static_cast<OrderType>(std::stoi(tok));
                std::getline(ss, tok,       ','); o.price    = std::stod(tok);
                std::getline(ss, tok,       ','); o.quantity = std::stoull(tok);
                std::getline(ss, tok);           o.timestamp= std::stoull(tok);

                inQ.enqueue(o);
            } })
        .detach();
  }

  engThread.join();
  return 0;
}
