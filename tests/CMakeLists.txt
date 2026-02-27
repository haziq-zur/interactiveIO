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

# =============================================================================
# Qt GUI Tests (only if Qt6 is available)
# =============================================================================

# Find Qt Test module and Widgets
find_package(Qt6 COMPONENTS Core Widgets Test QUIET)

if(Qt6Test_FOUND AND Qt6Widgets_FOUND)
    message(STATUS "Qt6 Test found - Building GUI tests")
    
    # Enable Qt features for GUI tests
    set(CMAKE_AUTOMOC ON)
    set(CMAKE_AUTOUIC ON)
    
    # GUI test sources
    set(GUI_TEST_SOURCES
        test_gui_mainwindow.cpp
        ${CMAKE_SOURCE_DIR}/gui/mainwindow.cpp
        ${CMAKE_SOURCE_DIR}/gui/mainwindow.h
        ${CMAKE_SOURCE_DIR}/gui/mainwindow.ui
        ${CMAKE_SOURCE_DIR}/gui/connectiondialog.cpp
        ${CMAKE_SOURCE_DIR}/gui/connectiondialog.h
        ${CMAKE_SOURCE_DIR}/gui/connectiondialog.ui
    )
    
    # Create GUI test executable
    add_executable(gui_tests ${GUI_TEST_SOURCES})
    
    # Link against Qt Test, instrument_core, and Qt6 modules
    target_link_libraries(gui_tests
        PRIVATE
        instrument_core
        Qt6::Core
        Qt6::Widgets
        Qt6::Test
    )
    
    # Include directories
    target_include_directories(gui_tests
        PRIVATE
        ${CMAKE_SOURCE_DIR}/include
        ${CMAKE_SOURCE_DIR}/gui
    )
    
    # Add test to CTest
    add_test(NAME GUITests COMMAND gui_tests)
    
    message(STATUS "GUI tests configured: gui_tests")
else()
    message(STATUS "Qt6 Test not found - GUI tests will not be built")
endif()

