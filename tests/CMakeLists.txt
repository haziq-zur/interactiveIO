# -----------------------------------------------------------------------------
# Core (non-GUI) unit tests
# -----------------------------------------------------------------------------

add_executable(scpi_tests
    core/test_scpi_utils.cpp
    core/test_scpi_connection.cpp
    core/test_visa_connection.cpp
    core/test_ieee4882.cpp
)

target_link_libraries(scpi_tests PRIVATE
    GTest::gtest_main
    GTest::gmock
    instrument_core
)

include(GoogleTest)
gtest_discover_tests(scpi_tests)

# -----------------------------------------------------------------------------
# Controller unit tests
# -----------------------------------------------------------------------------

add_executable(controller_tests
    controller/test_instrument_controller.cpp
)

target_link_libraries(controller_tests PRIVATE
    GTest::gtest_main
    GTest::gmock
    instrument_controller
)

gtest_discover_tests(controller_tests)

# -----------------------------------------------------------------------------
# REST API unit tests (only when the API library is available)
# -----------------------------------------------------------------------------

if(BUILD_API AND TARGET interactiveIO_api)
    add_executable(api_tests
        api/test_api_serialization.cpp
    )

    target_link_libraries(api_tests PRIVATE
        GTest::gtest_main
        interactiveIO_api
    )

    gtest_discover_tests(api_tests)

    message(STATUS "REST API tests configured: api_tests")
endif()

# -----------------------------------------------------------------------------
# Qt GUI tests (only when Qt6 and the GUI library are available)
# -----------------------------------------------------------------------------

if(BUILD_GUI AND Qt6_FOUND AND TARGET interactiveIO_gui)
    message(STATUS "Qt6 found - Building GUI tests")

    # MainWindow tests
    add_executable(gui_tests gui/test_gui_mainwindow.cpp)
    set_target_properties(gui_tests PROPERTIES AUTOMOC ON)
    target_link_libraries(gui_tests PRIVATE
        interactiveIO_gui
        Qt6::Test
    )
    add_test(NAME GUITests COMMAND gui_tests)

    # ConnectionDialog tests (USB / GPIB helpers, etc.)
    add_executable(connectiondialog_tests gui/test_gui_connectiondialog.cpp)
    set_target_properties(connectiondialog_tests PROPERTIES AUTOMOC ON)
    target_link_libraries(connectiondialog_tests PRIVATE
        interactiveIO_gui
        Qt6::Test
    )
    add_test(NAME ConnectionDialogTests COMMAND connectiondialog_tests)

    # Run the GUI tests fully headless so they require no display or user
    # interaction (suitable for CI/CD). The offscreen platform plugin renders
    # widgets without a window server. QT_QPA_PLATFORM_PLUGIN_PATH points the
    # test binaries at the Qt installation's platform plugins (qoffscreen),
    # which is not copied next to the executables by windeployqt.
    get_filename_component(_qt_prefix "${Qt6_DIR}/../../.." ABSOLUTE)
    set(_qt_platforms_dir "${_qt_prefix}/plugins/platforms")

    set_tests_properties(GUITests ConnectionDialogTests PROPERTIES
        ENVIRONMENT "QT_QPA_PLATFORM=offscreen;QT_QPA_PLATFORM_PLUGIN_PATH=${_qt_platforms_dir}"
    )

    message(STATUS "GUI tests configured: gui_tests, connectiondialog_tests")
else()
    message(STATUS "Qt6 GUI library not available - GUI tests will not be built")
endif()
