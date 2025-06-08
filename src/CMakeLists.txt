add_executable(engine
    main.cpp
)

# Find Boost.Asio (header-only)
find_package(Boost 1.66 REQUIRED COMPONENTS system)
target_include_directories(engine PRIVATE ${Boost_INCLUDE_DIRS})
target_link_libraries(engine PRIVATE Boost::system pthread) 
# pthread is for Linux; on macOS it’s a no-op.

target_include_directories(engine PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})