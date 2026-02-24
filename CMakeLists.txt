cmake_minimum_required(VERSION 3.15)

project(interactiveIO
    VERSION 1.0.0
    DESCRIPTION "Interactive SCPI instrument communication over TCP/IP"
    LANGUAGES CXX
)

# Set C++ standard
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Debug and Release configurations
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Choose the type of build (Debug or Release)" FORCE)
endif()

# Debug-specific flags
if(MSVC)
    # MSVC Debug flags
    set(CMAKE_CXX_FLAGS_DEBUG "/MDd /Zi /Ob0 /Od /RTC1")
    set(CMAKE_CXX_FLAGS_RELEASE "/MD /O2 /Ob2 /DNDEBUG")
    add_compile_definitions($<$<CONFIG:Debug>:_DEBUG>)
else()
    # GCC/Clang Debug flags
    set(CMAKE_CXX_FLAGS_DEBUG "-g -O0 -DDEBUG")
    set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG")
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
)

add_library(instrument_core ${INSTRUMENT_CORE_SOURCES})

# Include directories for the library
target_include_directories(instrument_core PUBLIC
    ${CMAKE_SOURCE_DIR}/include
)

# Platform-specific configurations for library
if(WIN32)
    # TCP/IP requires Winsock2
    target_link_libraries(instrument_core PUBLIC ws2_32)
    # Note: VISA is dynamically loaded at runtime, no link-time dependency
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

# Add unit tests subdirectory
add_subdirectory(tests)
