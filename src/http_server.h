#pragma once

#include <boost/asio.hpp>
#include "MatchingEngine.h"

namespace asio = boost::asio;

void run_http_server(asio::io_context& ioc,
                     unsigned short    port,
                     MatchingEngine&   engine);
