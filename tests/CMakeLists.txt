cmake_minimum_required(VERSION 3.15)

# Add test executable
add_executable(scpi_tests
    test_scpi_utils.cpp
    test_scpi_connection.cpp
    test_visa_connection.cpp
)

# Link against Google Test, GMock and the core library
target_link_libraries(scpi_tests
    PRIVATE
    GTest::gtest_main
    GTest::gmock
    instrument_core
)

# Include directories
target_include_directories(scpi_tests
    PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)

# Discover tests for CTest
include(GoogleTest)
gtest_discover_tests(scpi_tests)
