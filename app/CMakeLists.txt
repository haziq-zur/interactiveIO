# -----------------------------------------------------------------------------
# Console application
# -----------------------------------------------------------------------------

add_executable(interactiveIO
    console/main.cpp
)

target_link_libraries(interactiveIO PRIVATE instrument_core)

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
    )

    set_target_properties(interactiveIO_gui PROPERTIES
        AUTOMOC ON
        AUTOUIC ON
    )

    target_include_directories(interactiveIO_gui PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/gui
    )

    target_link_libraries(interactiveIO_gui PUBLIC
        instrument_core
        Qt6::Core
        Qt6::Widgets
    )

    # GUI executable: entry point + resources, linking the shared GUI library.
    add_executable(interactiveIO-gui WIN32
        gui/main_gui.cpp
        gui/resources.qrc
    )

    set_target_properties(interactiveIO-gui PROPERTIES
        AUTOMOC ON
        AUTORCC ON
    )

    target_link_libraries(interactiveIO-gui PRIVATE interactiveIO_gui)

    install(TARGETS interactiveIO-gui RUNTIME DESTINATION bin)

    message(STATUS "GUI application configured: interactiveIO-gui")
endif()
