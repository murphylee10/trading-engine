cmake_minimum_required(VERSION 3.15)
project(trading_engine_core LANGUAGES CXX)

cmake_policy(SET CMP0167 NEW)
find_package(Boost 1.66 REQUIRED COMPONENTS system)

find_path(RDKAFKA_INCLUDE_DIR
  NAMES rdkafka.h
  HINTS /opt/homebrew/include /usr/local/include
  PATH_SUFFIXES librdkafka
)

find_library(RDKAFKA_LIBRARY
  NAMES rdkafka
  HINTS /opt/homebrew/lib /usr/local/lib
)

find_library(RDKAFAKACPP_LIBRARY
  NAMES rdkafka++
  HINTS /opt/homebrew/lib /usr/local/lib
)

if (NOT RDKAFKA_INCLUDE_DIR OR NOT RDKAFKA_LIBRARY OR NOT RDKAFAKACPP_LIBRARY)
  message(FATAL_ERROR "Could not find librdkafka or its C++ wrapper;\n"
      "  Header: ${RDKAFKA_INCLUDE_DIR}\n"
      "  C library: ${RDKAFKA_LIBRARY}\n"
      "  C++ library: ${RDKAFAKACPP_LIBRARY}")
endif()

add_library(core
  OrderBook.cpp
  MatchingEngine.cpp
)

target_include_directories(core PUBLIC
  ${Boost_INCLUDE_DIRS}
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_SOURCE_DIR}/include
  ${RDKAFKA_INCLUDE_DIR}
)

target_link_libraries(core PUBLIC
  Boost::system
  pthread
  ${RDKAFKA_LIBRARY}
  ${RDKAFAKACPP_LIBRARY}
)

add_executable(engine
  main.cpp
  http_server.cpp
)

target_link_libraries(engine PRIVATE core)
