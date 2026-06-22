#ifndef IIO_REST_SERVER_H
#define IIO_REST_SERVER_H

#include <memory>
#include <mutex>
#include <string>

#include <nlohmann/json.hpp>

#include "instrument_controller.h"

// Forward declaration keeps the heavy cpp-httplib header out of this public
// interface; it is only included by the implementation translation unit.
namespace httplib { class Server; }

namespace iio {
namespace api {

// -----------------------------------------------------------------------------
// REST API frontend.
//
// This presentation layer exposes the InstrumentController over HTTP so the
// application can be driven from the command line (curl / scripts) or any HTTP
// client. Like the other frontends, it depends solely on the controller and
// never touches the backend transports directly.
// -----------------------------------------------------------------------------

// --- Pure mapping helpers (no sockets; unit-testable) ------------------------

// Serialises a controller Result into the canonical JSON response body.
nlohmann::json resultToJson(const Result& result);

// Chooses the HTTP status code that best matches a Result's outcome.
int httpStatusFor(const Result& result);

// Parses a Protocol from its wire name ("tcpip" / "visa", case-insensitive).
// Returns false when the value is unrecognised.
bool parseProtocol(const std::string& name, Protocol& out);

// Builds a ConnectionConfig from a parsed JSON request body. On failure returns
// false and fills `error` with a human-readable reason.
bool parseConnectionConfig(const nlohmann::json& body, ConnectionConfig& out,
                           std::string& error);

// Encodes raw bytes as standard (RFC 4648) base64 text.
std::string base64Encode(const std::vector<uint8_t>& data);

// --- HTTP server -------------------------------------------------------------

// Thin REST facade over an InstrumentController. Every instrument operation is
// serialised through a mutex because the single connection is a shared resource
// and the underlying server dispatches each request on its own thread.
class RestServer {
public:
    explicit RestServer(InstrumentController& controller);
    ~RestServer();

    RestServer(const RestServer&) = delete;
    RestServer& operator=(const RestServer&) = delete;

    // Binds the host/port and blocks serving requests until stop() is called.
    // Returns false when the address could not be bound.
    bool listen(const std::string& host, int port);

    // Stops a running server. Safe to call from another thread / signal handler.
    void stop();

private:
    void registerRoutes();

    InstrumentController& controller_;
    std::mutex mutex_;  // serialises all controller access
    std::unique_ptr<httplib::Server> server_;
};

} // namespace api
} // namespace iio

#endif // IIO_REST_SERVER_H
