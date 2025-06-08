# TradingEngine

A lightweight C++17‐based matching engine prototype demonstrating real‐time order matching, multi‐client support, and pluggable persistence.  Ideal as a résumé project for quant‐dev roles.

## 🚀 Features

- **Limit, Market & Cancel orders** with price–time priority matching  
- **Multi‐symbol** support via per‐symbol order books  
- **Concurrent clients** over Boost.Asio asynchronous TCP  
- **Write-ahead log** of orders & trades to flat files  
- **REST snapshot API** (Phase 3) for book state & recent trades  
- **Unit tests** for data models using Catch2  

## 🛠 Prerequisites

- **C++17 toolchain** (GCC ≥ 9 / Clang ≥ 10)  
- **CMake** ≥ 3.15  
- **Boost.System** (for Boost.Asio)  
- **pthread** (Linux/macOS)  
- **Catch2** single‐header in `tests/extern/catch.hpp`  
- *(Optional)* Docker & Docker Compose  
- *(Optional)* clang-format, clang-tidy  

## 🏗 Build & Test

```bash
# Clone & enter
git clone https://github.com/murphylee10/trading‐engine.git
cd trading‐engine

# Create build directory
mkdir build && cd build

# Configure
cmake ..

# Build engine + tests
make

# Run unit tests
ctest --output-on-failure
