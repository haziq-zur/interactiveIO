cmake_minimum_required(VERSION 3.15)

project(interactiveIO
    VERSION 1.0.0
    DESCRIPTION "Interactive SCPI instrument communication over TCP/IP and VISA"
    LANGUAGES CXX
)

# -----------------------------------------------------------------------------
# Global configuration
# -----------------------------------------------------------------------------

# Set C++ standard (Qt6 requires C++17)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Default to a Release build
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Choose the type of build (Debug or Release)" FORCE)
endif()

# Option for static runtime linking (makes executables self-contained)
option(STATIC_RUNTIME "Link with static runtime library" ON)

# Option to build the Qt6 GUI application (enabled by default)
option(BUILD_GUI "Build Qt6 GUI application" ON)

# Option to build the REST API application (enabled by default)
option(BUILD_API "Build REST API application" ON)

# Compiler / runtime flags
if(MSVC)
    if(STATIC_RUNTIME)
        set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
        set(CMAKE_CXX_FLAGS_DEBUG "/MTd /Zi /Ob0 /Od /RTC1")
        set(CMAKE_CXX_FLAGS_RELEASE "/MT /O2 /Ob2 /DNDEBUG")
    else()
        set(CMAKE_CXX_FLAGS_DEBUG "/MDd /Zi /Ob0 /Od /RTC1")
        set(CMAKE_CXX_FLAGS_RELEASE "/MD /O2 /Ob2 /DNDEBUG")
    endif()
    add_compile_definitions($<$<CONFIG:Debug>:_DEBUG>)
else()
    set(CMAKE_CXX_FLAGS_DEBUG "-g -O0 -DDEBUG")
    set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG")
    if(STATIC_RUNTIME)
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -static-libgcc -static-libstdc++")
    endif()
endif()

# Common runtime output directory for all executables
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/$<CONFIG>")

# -----------------------------------------------------------------------------
# Dependencies
# -----------------------------------------------------------------------------

enable_testing()

# Google Test (downloaded on demand)
include(FetchContent)
FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG v1.14.0
)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

# REST API dependencies (header-only, downloaded on demand).
if(BUILD_API)
    FetchContent_Declare(
        httplib
        GIT_REPOSITORY https://github.com/yhirose/cpp-httplib.git
        GIT_TAG v0.15.3
    )
    FetchContent_Declare(
        nlohmann_json
        GIT_REPOSITORY https://github.com/nlohmann/json.git
        GIT_TAG v3.11.3
    )
    FetchContent_MakeAvailable(httplib nlohmann_json)
    message(STATUS "REST API dependencies ready: cpp-httplib, nlohmann/json")
endif()

# Qt6 is optional - the GUI and GUI tests are skipped when it is missing.
if(BUILD_GUI)
    find_package(Qt6 COMPONENTS Core Widgets Test QUIET)
    if(Qt6_FOUND)
        message(STATUS "Qt6 found - GUI application and GUI tests will be built")
    else()
        message(WARNING "Qt6 not found - GUI application and GUI tests will be skipped")
        message(WARNING "Set Qt6_DIR to your Qt installation, e.g. C:/Qt/6.6.1/msvc2019_64")
    endif()
endif()

# -----------------------------------------------------------------------------
# Subdirectories
# -----------------------------------------------------------------------------

# -----------------------------------------------------------------------------
# Subdirectories (tiered architecture: backend -> controller -> frontend)
# -----------------------------------------------------------------------------

add_subdirectory(backend)     # BE: instrument_core transport library
add_subdirectory(controller)  # bridge: instrument_controller library
add_subdirectory(frontend)    # FE: console + GUI applications
add_subdirectory(tests)       # unit tests (backend, controller, GUI)

# -----------------------------------------------------------------------------
# Documentation install
# -----------------------------------------------------------------------------

install(FILES
    "${CMAKE_SOURCE_DIR}/README.md"
    "${CMAKE_SOURCE_DIR}/docs/VISA_GUIDE.md"
    "${CMAKE_SOURCE_DIR}/docs/USB_GUIDE.md"
    DESTINATION docs
)
