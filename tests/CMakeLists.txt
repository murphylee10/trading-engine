include_directories(${CMAKE_CURRENT_SOURCE_DIR}/extern)

add_executable(test_order
    test_order.cpp
    test_order_book.cpp
    test_matching_engine.cpp
)

target_link_libraries(test_order
  PRIVATE
    core
    pthread
)

add_test(NAME AllUnitTests COMMAND test_order)
