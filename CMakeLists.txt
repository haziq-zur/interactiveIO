cmake_minimum_required(VERSION 3.15)

project(interactiveIO
    VERSION 1.0.0
    DESCRIPTION "Interactive SCPI instrument communication over TCP/IP"
    LANGUAGES CXX
)

# Set C++ standard (Qt6 requires C++17)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Debug and Release configurations
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Choose the type of build (Debug or Release)" FORCE)
endif()

# Option for static runtime linking (makes executable self-contained)
option(STATIC_RUNTIME "Link with static runtime library" ON)

# Debug-specific flags
if(MSVC)
    if(STATIC_RUNTIME)
        # Static runtime - no external DLL dependencies for MSVC runtime
        set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
        set(CMAKE_CXX_FLAGS_DEBUG "/MTd /Zi /Ob0 /Od /RTC1")
        set(CMAKE_CXX_FLAGS_RELEASE "/MT /O2 /Ob2 /DNDEBUG")
    else()
        # Dynamic runtime - requires vcruntime DLLs
        set(CMAKE_CXX_FLAGS_DEBUG "/MDd /Zi /Ob0 /Od /RTC1")
        set(CMAKE_CXX_FLAGS_RELEASE "/MD /O2 /Ob2 /DNDEBUG")
    endif()
    add_compile_definitions($<$<CONFIG:Debug>:_DEBUG>)
else()
    # GCC/Clang flags
    set(CMAKE_CXX_FLAGS_DEBUG "-g -O0 -DDEBUG")
    set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG")
    if(STATIC_RUNTIME)
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -static-libgcc -static-libstdc++")
    endif()
endif()

# Include directories
include_directories(${CMAKE_SOURCE_DIR}/include)

# Enable testing
enable_testing()

# Include FetchContent for downloading Google Test
include(FetchContent)
FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG v1.14.0
)
# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

# Create library with instrument connection implementations
# Both TCP/IP and VISA protocols are always included
set(INSTRUMENT_CORE_SOURCES
    src/tcpip_connection.cpp
    src/visa_connection.cpp
    src/platform/socket_wrapper.cpp
    src/platform/dynamic_loader.cpp
)

add_library(instrument_core ${INSTRUMENT_CORE_SOURCES})

# Include directories for the library
target_include_directories(instrument_core PUBLIC
    ${CMAKE_SOURCE_DIR}/include
)

# Platform-specific configurations for library
if(WIN32)
    # TCP/IP requires Winsock2 on Windows
    target_link_libraries(instrument_core PUBLIC ws2_32)
    # Note: VISA is dynamically loaded at runtime, no link-time dependency
elseif(UNIX)
    # On Linux/macOS, sockets are built-in
    # Link with dl library for dynamic loading (dlopen/dlsym)
    target_link_libraries(instrument_core PUBLIC ${CMAKE_DL_LIBS})
endif()

# Add the main executable with unified protocol menu
add_executable(interactiveIO
    src/main.cpp
)

# Link the core library to the executable
target_link_libraries(interactiveIO PRIVATE instrument_core)

# Platform-specific configurations
if(WIN32)
    # Set subsystem to console
    if(MSVC)
        set_target_properties(interactiveIO PROPERTIES
            LINK_FLAGS "/SUBSYSTEM:CONSOLE"
        )
    endif()
endif()

# Set output directories
set_target_properties(interactiveIO PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/$<CONFIG>"
)

# Installation rules (optional)
install(TARGETS interactiveIO
    RUNTIME DESTINATION bin
)

# Install documentation
install(FILES 
    "${CMAKE_SOURCE_DIR}/README.md"
    "${CMAKE_SOURCE_DIR}/REFACTORING_SUMMARY.md"
    "${CMAKE_SOURCE_DIR}/VISA_GUIDE.md"
    DESTINATION docs
)

# Add unit tests subdirectory
add_subdirectory(tests)

# =============================================================================
# Qt6 GUI Application
# =============================================================================

# Option to build GUI (enabled by default)
option(BUILD_GUI "Build Qt6 GUI application" ON)

if(BUILD_GUI)
    # Find Qt6 packages
    find_package(Qt6 COMPONENTS Core Widgets Test QUIET)
    
    if(Qt6_FOUND)
        message(STATUS "Qt6 found - Building GUI application")
        
        # Enable automoc, autouic, and autorcc
        set(CMAKE_AUTOMOC ON)
        set(CMAKE_AUTOUIC ON)
        set(CMAKE_AUTORCC ON)
        
        # GUI source files
        set(GUI_SOURCES
            gui/main_gui.cpp
            gui/mainwindow.cpp
            gui/mainwindow.h
            gui/mainwindow.ui
            gui/connectiondialog.cpp
            gui/connectiondialog.h
            gui/connectiondialog.ui
            gui/resources.qrc
        )
        
        # Create GUI executable
        add_executable(interactiveIO-gui WIN32 ${GUI_SOURCES})
        
        # Link against instrument_core library and Qt6
        target_link_libraries(interactiveIO-gui PRIVATE 
            instrument_core
            Qt6::Core
            Qt6::Widgets
        )
        
        # Include directories
        target_include_directories(interactiveIO-gui PRIVATE
            ${CMAKE_SOURCE_DIR}/include
            ${CMAKE_SOURCE_DIR}/gui
        )
        
        # Set output directories
        set_target_properties(interactiveIO-gui PROPERTIES
            RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/$<CONFIG>"
        )
        
        # Installation rules for GUI
        install(TARGETS interactiveIO-gui
            RUNTIME DESTINATION bin
        )
        
        message(STATUS "GUI application configured: interactiveIO-gui")
    else()
        message(WARNING "Qt6 not found - GUI application will not be built. Install Qt6 to enable GUI.")
        message(WARNING "Set Qt6_DIR to Qt installation path, e.g., C:/Qt/6.6.1/msvc2019_64")
    endif()
endif()
