# -----------------------------------------------------------------------------
# Frontend (FE): user-facing applications
#
# These talk only to the controller (instrument_controller); they never include
# or link the backend transports directly. This keeps presentation concerns
# fully isolated from the instrument communication logic.
# -----------------------------------------------------------------------------

# Windows-only resource that embeds the application icon (.ico) into each
# shipped executable. Left empty on other platforms and for the test targets.
if(WIN32)
    set(IIO_APP_ICON_RC "${CMAKE_CURRENT_SOURCE_DIR}/gui/resources/app_icon.rc")
else()
    set(IIO_APP_ICON_RC "")
endif()

# -----------------------------------------------------------------------------
# Console application
# -----------------------------------------------------------------------------

add_executable(interactiveIO
    console/main.cpp
    ${IIO_APP_ICON_RC}
)

target_link_libraries(interactiveIO PRIVATE instrument_controller)

if(WIN32 AND MSVC)
    set_target_properties(interactiveIO PROPERTIES
        LINK_FLAGS "/SUBSYSTEM:CONSOLE"
    )
endif()

# On MinGW the console app has no Qt dependency, so link the runtime fully
# statically (libgcc, libstdc++ and libwinpthread) to produce a self-contained
# executable that does not require libwinpthread-1.dll etc. at runtime.
if(STATIC_RUNTIME AND MINGW)
    target_link_options(interactiveIO PRIVATE -static)
endif()

install(TARGETS interactiveIO RUNTIME DESTINATION bin)

# -----------------------------------------------------------------------------
# Qt6 GUI application (optional)
# -----------------------------------------------------------------------------

if(BUILD_GUI AND Qt6_FOUND)
    # Shared GUI library so the widgets can be reused by the GUI tests without
    # being recompiled in every test target.
    add_library(interactiveIO_gui STATIC
        gui/mainwindow.cpp
        gui/mainwindow.h
        gui/mainwindow.ui
        gui/connectiondialog.cpp
        gui/connectiondialog.h
        gui/connectiondialog.ui
        gui/settingsdialog.cpp
        gui/settingsdialog.h
        gui/theme.cpp
        gui/theme.h
        gui/loadingoverlay.cpp
        gui/loadingoverlay.h
        gui/errorhandler.cpp
        gui/errorhandler.h
        gui/logcrypto.cpp
        gui/logcrypto.h
    )

    set_target_properties(interactiveIO_gui PROPERTIES
        AUTOMOC ON
        AUTOUIC ON
    )

    target_include_directories(interactiveIO_gui PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/gui
    )

    # The GUI links the controller (not the backend) plus Qt.
    target_link_libraries(interactiveIO_gui PUBLIC
        instrument_controller
        Qt6::Core
        Qt6::Widgets
    )

    # Link Qt Svg when available so QIcon can render the SVG app icon. Guarded
    # so the GUI still builds on Qt installs without the Svg module.
    if(TARGET Qt6::Svg)
        target_link_libraries(interactiveIO_gui PUBLIC Qt6::Svg)
        target_compile_definitions(interactiveIO_gui PUBLIC IIO_HAVE_QTSVG)
    endif()

    # GUI executable: entry point + resources, linking the shared GUI library.
    add_executable(interactiveIO-gui WIN32
        gui/main_gui.cpp
        gui/resources.qrc
        ${IIO_APP_ICON_RC}
    )

    set_target_properties(interactiveIO-gui PROPERTIES
        AUTOMOC ON
        AUTORCC ON
    )

    target_link_libraries(interactiveIO-gui PRIVATE interactiveIO_gui)

    install(TARGETS interactiveIO-gui RUNTIME DESTINATION bin)

    message(STATUS "GUI application configured: interactiveIO-gui")
endif()

# -----------------------------------------------------------------------------
# REST API application (optional)
# -----------------------------------------------------------------------------

if(BUILD_API)
    # Shared API library so the routes/serialisers can be reused by unit tests
    # without recompiling them into every test target.
    add_library(interactiveIO_api STATIC
        api/rest_server.cpp
        api/rest_server.h
    )

    target_include_directories(interactiveIO_api PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/api
    )

    # The API links the controller (not the backend) plus the HTTP/JSON libs.
    target_link_libraries(interactiveIO_api PUBLIC
        instrument_controller
        httplib::httplib
        nlohmann_json::nlohmann_json
    )

    add_executable(interactiveIO-api
        api/main_api.cpp
        ${IIO_APP_ICON_RC}
    )

    target_link_libraries(interactiveIO-api PRIVATE interactiveIO_api)

    if(WIN32 AND MSVC)
        set_target_properties(interactiveIO-api PROPERTIES
            LINK_FLAGS "/SUBSYSTEM:CONSOLE"
        )
    endif()

    if(STATIC_RUNTIME AND MINGW)
        target_link_options(interactiveIO-api PRIVATE -static)
    endif()

    install(TARGETS interactiveIO-api RUNTIME DESTINATION bin)

    message(STATUS "REST API application configured: interactiveIO-api")
endif()

