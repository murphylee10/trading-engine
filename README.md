# Trading Engine & Metrics Pipeline

A **C++17-based, high-performance matching engine** with real-time metrics streaming, REST snapshots, and observable dashboards.  
It demonstrates:

- Low-latency order matching in C++ with Boost.Asio
- Decoupled streaming via Apache Kafka
- Time-series metrics in InfluxDB and Grafana dashboards
- Easy local deployment with Docker Compose
- REST-style monitoring with Boost.Beast + nlohmann/json
- Unit tests with Catch2

---

## Features

- Limit, Market & Cancel orders (price–time priority)
- Multi-symbol order-books
- Concurrent TCP order intake (port `9000`)
- REST snapshot API (port `8080`) for book & trades
- Kafka topics: `orders`, `trades`, `metrics`
- Metrics: order latency & throughput → InfluxDB
- Grafana dashboard for real-time visualization
- Unit tests (Catch2) covering core logic

## 🛠 Prerequisites

- C++17 toolchain (GCC ≥ 9 / Clang ≥ 10)
- CMake ≥ 3.15
- Boost.System (for Asio)
- librdkafka & librdkafka++ (C++ Kafka client)
- nlohmann/json (single-header)
- moodycamel::ConcurrentQueue (single-header)
- Docker & Docker Compose
- Python 3.7+ (for metrics consumer)

---

## Setup

### 1. Install System Dependencies

```bash
brew update
brew install cmake boost librdkafka librdkafka++ pkg-config
brew install nlohmann-json
brew install docker docker-compose
```

(alternative setups for other OSes are definitely possible, but I haven't done them)

### 3. Launch Docker Compose Stack

```bash
docker-compose up -d
```

### 4. Build & Run

```
mkdir -p build && cd build
cmake ..
make
ctest --output-on-failure
./src/engine
```

### 5. Run Metrics Consumer

From project root (in a separate session from running engine instance):

```
python3 -m venv .venv
source .venv/bin/activate
pip install --upgrade pip
pip install kafka-python influxdb-client
chmod +x metrics_consumer.py
```

Here's some ways to play around with it:

Send orders via netcat

```bash
nc localhost 9000
# Paste:
1001,1,AAPL,0,0,150.0,5,1650000000000
1002,2,AAPL,1,0,150.0,3,1650000000100
```

Query REST API

```
curl "http://localhost:8080/book/AAPL?depth=5"
curl "http://localhost:8080/trades/AAPL?limit=10"
```

Watch Kafka topics

```
kcat -b localhost:9092 -t orders
kcat -b localhost:9092 -t trades
kcat -b localhost:9092 -t metrics
```

## Next Steps

- Live push via WebSockets or Server-Sent Events (SSE)
- Multi-node scaling with Kubernetes / Helm
- Real exchange adapters using FIX
- Authentication & order-management UI
