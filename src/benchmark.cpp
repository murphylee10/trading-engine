#include <iostream>
#include <vector>
#include <algorithm>
#include <chrono>
#include "MatchingEngine.h"
#include "Order.h"

using namespace std;
using clk = chrono::high_resolution_clock;
using ns  = chrono::nanoseconds;

int main(int argc, char* argv[]) {
    const size_t N = (argc>1 ? stoull(argv[1]) : 1'000'000);
    MatchingEngine engine;

    vector<Order> orders;
    orders.reserve(N);
    for (size_t i = 0; i < N; ++i) {
        Order o;
        o.orderId   = i+1;
        o.accountId = 1;
        o.symbol    = "AAPL";
        o.side      = (i%2==0 ? Side::BUY : Side::SELL);
        o.type      = OrderType::LIMIT;
        o.price     = 100.0 + (i%100)*0.01;
        o.quantity  = 1;
        o.timestamp = chrono::duration_cast<ns>(clk::now().time_since_epoch()).count();
        orders.push_back(o);
    }

    vector<uint64_t> latencies;
    latencies.reserve(N);

    auto start_all = clk::now();
    for (auto& o : orders) {
        auto t0 = clk::now();
        engine.onNewOrder(o);
        auto t1 = clk::now();
        latencies.push_back(
          chrono::duration_cast<ns>(t1 - t0).count()
        );
    }
    auto end_all = clk::now();

    sort(latencies.begin(), latencies.end());
    auto p50 = latencies[N/2];
    auto p90 = latencies[size_t(N*0.90)];
    auto p99 = latencies[size_t(N*0.99)];
    double throughput = double(N) / chrono::duration<double>(end_all - start_all).count();

    cout << "Ran " << N << " orders in "
         << chrono::duration<double>(end_all - start_all).count() << " s\n";
    cout << " Throughput = " << throughput << " orders/s\n";
    cout << " Latency p50  = " << p50  << " ns\n";
    cout << " Latency p90  = " << p90  << " ns\n";
    cout << " Latency p99  = " << p99  << " ns\n";
    return 0;
}
