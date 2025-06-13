# Trading Engine & Metrics Pipeline

A C++17 trading‐engine prototype with:

- **Limit & Market orders** with price–time priority
- **TCP ingestion** of CSV orders (`orderId,accountId,symbol,side,type,price,quantity,timestamp\n`)
- **REST API** (Boost.Beast) for order‐book snapshots and recent trades
- **CORS support** so any frontend can fetch `/book/{symbol}` and `/trades/{symbol}`
- **Realtime metrics** (order latency & throughput) → Kafka → InfluxDB → Grafana
- **Simple JavaScript dashboard** polling the REST API

## Features

- Limit, Market & Cancel orders (price–time priority)
- Multi-symbol order-books
- Concurrent TCP order intake (port `9000`)
- REST snapshot API (port `8080`) for book & trades
- Kafka topics: `orders`, `trades`, `metrics`
- Metrics: order latency & throughput → InfluxDB
- Grafana dashboard for real-time visualization
- Unit tests (Catch2) covering core logic

## Prerequisites

- C++17 toolchain (GCC ≥ 9 / Clang ≥ 10)
- CMake ≥ 3.15
- Boost.System (for Asio)
- librdkafka & librdkafka++ (C++ Kafka client)
- nlohmann/json (single-header)
- moodycamel::ConcurrentQueue (single-header)
- Docker & Docker Compose
- Python 3.7+ (for metrics consumer)

### 1. System prerequisites

- **C++ toolchain**: clang-14 or gcc-9+, CMake ≥ 3.15
- **Boost** (headers + System)
- **librdkafka** (for Kafka C++ producer)
- **Python 3** (for feeders & metrics bridge)
- **Docker & docker-compose**

For example, with Mac:

```bash
brew update
brew install cmake boost librdkafka librdkafka++ pkg-config
brew install nlohmann-json
brew install docker docker-compose
```

(alternative setups for other OSes are definitely possible, but I haven't done them)

### 2. Bring up the data platform

```bash
docker-compose up -d
```

This will launch:

- Zookeeper @2181
- Kafka @9092
- InfluxDB 2.x @8086 (admin/admin, bucket=metrics, org=myorg)
- Grafana @3000 (default admin/admin)

### 3. Build & run the engine

```bash
mkdir build && cd build
cmake ..
make
./src/engine
```

- Listens for orders over TCP 9000
- Serves REST on HTTP 8080

### 4. Build & Run

```bash
mkdir -p build && cd build
cmake ..
make
ctest --output-on-failure
./src/engine
```

### 5. Start the metrics bridge

Create a virtualenv (optional):

```bash
python3 -m venv .venv
source .venv/bin/activate
pip install kafka-python influxdb-client
chmod +x ../metrics_consumer.py
```

Then:

```bash
./metrics_consumer.py
```

Consumes the Kafka metrics topic and writes into InfluxDB.

### 6. Drive the engine with simulated orders

a) Via nc

```
echo "1001,1,AAPL,0,0,150.00,5,1650000000000" | nc localhost 9000
echo "1002,2,AAPL,1,0,150.00,3,1650000000100" | nc localhost 9000
```

b) Automated script

```bash
./feed_orders.py --host localhost --port 9000 \
                 --symbols AAPL,GOOG,TSLA \
                 --rate 5 --limit 100
chmod +x feed_orders.py
```

Streams random orders (5 Hz, 100 total by default).

### 7. Launch the dashboard

Serve the web/ folder on port 8000:

```bash
cd web
python3 -m http.server 8000
```

Open http://localhost:8000 in your browser:

- Symbol dropdown (AAPL | GOOG | TSLA)
- Order‐book snapshot (top 10 bids)
- Recent trades (last 10)
- Auto-refresh every 2 s
