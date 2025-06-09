# src/CMakeLists.txt
find_package(Boost 1.66 REQUIRED COMPONENTS system)

# 1) Core library of matching logic
add_library(core
    OrderBook.cpp
    MatchingEngine.cpp
)

# Public include paths and link deps for anyone linking core
target_include_directories(core
  PUBLIC
    ${Boost_INCLUDE_DIRS}            # for Boost.System / Asio
    ${CMAKE_CURRENT_SOURCE_DIR}      # src/ (OrderBook.h, MatchingEngine.h, etc.)
    ${CMAKE_SOURCE_DIR}/include      # concurrentqueue.h
)
target_link_libraries(core
  PUBLIC
    Boost::system
    pthread
)

# 2) Engine executable
add_executable(engine
    main.cpp
)

# Engine only needs to link core (which brings in its includes & libs)
target_link_libraries(engine
  PRIVATE
    core
)
