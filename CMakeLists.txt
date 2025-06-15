cmake_minimum_required(VERSION 3.15)
project(trading_engine VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Enable warnings and clang-tidy if installed
if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# Add subdirectories
add_subdirectory(src)
enable_testing()
add_subdirectory(tests)

add_executable(benchmark src/benchmark.cpp)
target_link_libraries(benchmark PRIVATE core)