#ifndef IIO_CLI_API_H
#define IIO_CLI_API_H

#include <ostream>
#include <string>
#include <vector>

#include <nlohmann/json.hpp>

#include "instrument_controller.h"

// -----------------------------------------------------------------------------
// Command-line API frontend.
//
// This presentation layer exposes the InstrumentController as a one-shot CLI
// tool: a single invocation opens a connection, performs one action (send a
// command, capture an image, clear the device, ...), prints a JSON result to
// stdout and exits. Like the other frontends, it depends solely on the
// controller and never touches the backend transports directly.
//
// Example:
//   interactiveIO-api --protocol tcpip --address 192.168.1.100 --port 5025 \
//       --command "*IDN?"
// -----------------------------------------------------------------------------

namespace iio {
namespace api {

// --- Pure mapping helpers (no I/O; unit-testable) ----------------------------

// Serialises a controller Result into the canonical JSON response body.
nlohmann::json resultToJson(const Result& result);

// Maps a Result to the process exit code: 0 on success, 2 for invalid input /
// usage errors, and 1 for every other failure.
int exitCodeFor(const Result& result);

// Parses a Protocol from its name ("tcpip" / "visa", case-insensitive).
// Returns false when the value is unrecognised.
bool parseProtocol(const std::string& name, Protocol& out);

// Builds a ConnectionConfig from a parsed JSON request body. On failure returns
// false and fills `error` with a human-readable reason.
bool parseConnectionConfig(const nlohmann::json& body, ConnectionConfig& out,
                           std::string& error);

// Encodes raw bytes as standard (RFC 4648) base64 text.
std::string base64Encode(const std::vector<uint8_t>& data);

// --- CLI driver --------------------------------------------------------------

// Parses `args` (the program arguments, excluding argv[0]), drives the
// controller accordingly, and writes a JSON object to `out` (diagnostics go to
// `err`). Returns the process exit code. This is the testable entry point used
// by main().
int runCli(InstrumentController& controller,
           const std::vector<std::string>& args,
           std::ostream& out,
           std::ostream& err);

} // namespace api
} // namespace iio

#endif // IIO_CLI_API_H
