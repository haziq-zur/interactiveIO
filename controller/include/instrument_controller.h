#ifndef INSTRUMENT_CONTROLLER_H
#define INSTRUMENT_CONTROLLER_H

#include <memory>
#include <string>

#include "instrument_connection.h"

namespace iio {

// Supported instrument transport protocols, exposed to the frontend so it never
// has to know about the concrete backend connection classes.
enum class Protocol {
    TCPIP,
    VISA
};

// Parameters needed to open a connection. For TCP/IP, `address` is the host IP
// and `port` is the TCP port. For VISA, `address` is the full resource string
// and `port` is ignored.
struct ConnectionConfig {
    Protocol protocol = Protocol::TCPIP;
    std::string address;
    int port = 5025;
};

// User-configurable parameters for the active communication channel. These are
// applied by the controller to every command it dispatches, so the frontends
// can expose a single "settings" surface without knowing transport details.
struct CommunicationSettings {
    // End-of-line sequence appended to each command before it is sent
    // (e.g. "\n", "\r", "\r\n", or "" for none).
    std::string eol = "\n";
    // Maximum time, in seconds, to wait for a query response.
    int responseTimeoutSeconds = 10;
    // When true, the controller measures how long each command takes to
    // complete (request sent -> response/ack received) and reports it via
    // Result.elapsedMs. Enabled by default.
    bool showResponseTime = true;
};

// Uniform result type returned by every controller operation. This keeps the
// frontend free of backend error-handling details: it only inspects `success`,
// shows `message`, and (for queries) reads `response`.
struct Result {
    bool success = false;       // operation outcome
    std::string message;        // human-readable status / error description
    std::string response;       // instrument reply (queries only)
    bool hasResponse = false;   // true when `response` was populated
    double elapsedMs = 0.0;     // time to complete the command, in milliseconds
};

// InstrumentController is the integration layer between the user-facing
// frontends (GUI / console) and the backend transport implementations. It owns
// the connection lifecycle, dispatches commands, and translates low-level
// backend errors into structured Result values. It has no UI dependency, so it
// can be reused by any frontend and unit-tested in isolation.
class InstrumentController {
public:
    InstrumentController();
    ~InstrumentController();

    InstrumentController(const InstrumentController&) = delete;
    InstrumentController& operator=(const InstrumentController&) = delete;

    // --- Connection lifecycle -------------------------------------------------

    // Creates the appropriate backend connection, initializes it and connects
    // using the supplied configuration. Any existing connection is closed first.
    Result connect(const ConnectionConfig& config);

    // Closes and releases the current connection (safe to call when not
    // connected).
    Result disconnect();

    bool isConnected() const;

    // True when the backend transport for `protocol` is usable on this system
    // (e.g. whether a VISA runtime is installed).
    bool isProtocolAvailable(Protocol protocol) const;

    // --- Command dispatch -----------------------------------------------------

    // Sends a single SCPI command. The configured EOL sequence is appended
    // before sending. When the command is a query (contains '?'), the
    // instrument response is read back and returned in Result.response with
    // hasResponse == true. Otherwise only the send outcome is reported.
    // Pass a non-negative timeoutSeconds to override the configured response
    // timeout for this single call; -1 uses the configured value.
    Result execute(const std::string& command, int timeoutSeconds = -1);

    // --- VISA-specific capabilities (no-ops / errors for other protocols) -----

    // Sets the VISA I/O timeout in milliseconds.
    Result setTimeout(unsigned int timeoutMs);

    // Clears the instrument's I/O buffers (VISA device clear).
    Result clearDevice();

    // --- Communication settings ----------------------------------------------

    // Replaces the active communication settings (EOL sequence + response
    // timeout). When connected via VISA, the I/O timeout is updated to match.
    void setCommunicationSettings(const CommunicationSettings& settings);
    const CommunicationSettings& communicationSettings() const;

    // --- Introspection --------------------------------------------------------

    std::string connectionInfo() const;
    std::string connectionType() const;
    Protocol currentProtocol() const;
    std::string lastError() const;

private:
    static std::unique_ptr<IInstrumentConnection> createConnection(Protocol protocol);

    std::unique_ptr<IInstrumentConnection> connection_;
    Protocol currentProtocol_ = Protocol::TCPIP;
    std::string lastError_;
    CommunicationSettings settings_;
};

// Stateless helpers for validating user input and building VISA resource
// strings. Exposing them here lets the frontends depend solely on the
// controller header instead of reaching into backend transport headers.
namespace resource {

bool isValidIPAddress(const std::string& ip);
bool isValidResourceString(const std::string& resourceString);
std::string buildTCPIPResource(const std::string& ip);
std::string buildSocketResource(const std::string& ip, int port);
std::string buildGPIBResource(int board, int address);
std::string interfaceType(const std::string& resourceString);

} // namespace resource

} // namespace iio

#endif // INSTRUMENT_CONTROLLER_H
