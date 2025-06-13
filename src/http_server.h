#pragma once
#include <boost/asio.hpp>
#include "MatchingEngine.h"

namespace asio = boost::asio;

/// Runs a blocking loop that serves:
///  - GET /book/{symbol}?depth={n}   → JSON order‐book snapshot
///  - GET /trades/{symbol}?limit={n} → JSON recent trades
void run_http_server(asio::io_context&  ioc,
                     unsigned short     port,
                     MatchingEngine&    engine);
