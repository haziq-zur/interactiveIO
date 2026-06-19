# -----------------------------------------------------------------------------
# Controller: instrument_controller
#
# The integration layer between the frontends (GUI / console) and the backend
# transports. Owns the connection lifecycle and command dispatch, exposing a
# small, UI-agnostic API that returns structured results. Links the backend but
# has no Qt dependency, so it is shared by every frontend and unit-tested on its
# own.
# -----------------------------------------------------------------------------

add_library(instrument_controller
    instrument_controller.cpp
    include/instrument_controller.h
)

target_include_directories(instrument_controller PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(instrument_controller PUBLIC
    instrument_core
)
