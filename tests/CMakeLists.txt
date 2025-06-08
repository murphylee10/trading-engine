# Locate Catch2 header
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/extern)

add_executable(test_order
    test_order.cpp
)

target_link_libraries(test_order PRIVATE pthread)
add_test(NAME OrderModelTest COMMAND test_order)