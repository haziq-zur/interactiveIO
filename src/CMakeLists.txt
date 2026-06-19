# -----------------------------------------------------------------------------
# instrument_core : transport + platform abstraction library
# -----------------------------------------------------------------------------

add_library(instrument_core
    core/tcpip_connection.cpp
    core/visa_connection.cpp
    platform/socket_wrapper.cpp
    platform/dynamic_loader.cpp
)

# Public headers live in the top-level include/ directory.
target_include_directories(instrument_core PUBLIC
    ${CMAKE_SOURCE_DIR}/include
)

# Platform-specific link dependencies
if(WIN32)
    # TCP/IP requires Winsock2 on Windows.
    # VISA is loaded dynamically at runtime, so there is no link-time dependency.
    target_link_libraries(instrument_core PUBLIC ws2_32)
elseif(UNIX)
    # On Linux/macOS, sockets are built-in; dl provides dlopen/dlsym.
    target_link_libraries(instrument_core PUBLIC ${CMAKE_DL_LIBS})
endif()
