#ifndef INSTRUMENT_CONTROLLER_H
#define INSTRUMENT_CONTROLLER_H

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

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

// Classifies every failure the controller can report so the frontends can react
// (and present) consistently without parsing message strings.
enum class ErrorCode {
    None = 0,            // no error (successful operation)
    NotConnected,        // an operation needs an open connection but there is none
    UnsupportedProtocol, // requested protocol has no backend implementation
    ProtocolUnavailable, // backend runtime/library is missing (e.g. no VISA)
    InitializationFailed,// transport failed to initialize
    ConnectionFailed,    // could not open the connection
    SendFailed,          // a command could not be written to the instrument
    NoResponse,          // a query produced no data (timeout or read error)
    OperationUnsupported,// operation is not valid for the active transport
    ClearFailed,         // VISA device clear failed
    InvalidInput,        // caller-supplied parameters were rejected
    BinaryReadFailed,    // a binary/image read failed at the transport level
    MalformedBlock,      // the IEEE 488.2 binary block could not be parsed
    UnsupportedImageFormat, // the captured data is not a recognised image
    ResponseTooLarge     // the response exceeded the configured size cap
};

// How serious a Result is, used by frontends to choose presentation (icon,
// colour, whether to interrupt the user with a modal dialog).
enum class Severity {
    Info,    // informational / success
    Warning, // recoverable; the session continues (e.g. query timeout)
    Error    // operation failed and needs user attention
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
    ErrorCode code = ErrorCode::None;     // structured failure classification
    Severity severity = Severity::Info;   // how the frontend should present it
};

// Parameters for capturing a binary image (screen capture) from the instrument.
struct ImageRequest {
    // SCPI command that makes the instrument return an image as an IEEE 488.2
    // binary block (e.g. ":DISPlay:DATA? PNG" or ":DISPlay:DATA?").
    std::string command;
    // Expected/desired format hint ("png", "bmp", "jpg"). When empty the format
    // is inferred from the returned data's magic bytes.
    std::string format;
    // Response timeout in seconds; -1 uses the configured value.
    int timeoutSeconds = -1;
    // Safety cap on the number of bytes to accept (default 32 MiB).
    size_t maxBytes = 32u * 1024u * 1024u;
};

// The decoded result of a successful image capture.
struct ImageCapture {
    std::vector<uint8_t> data;  // raw image bytes (payload only, header stripped)
    std::string format;         // detected format ("png", "bmp", "jpg", or "bin")
    double elapsedMs = 0.0;     // time to complete the capture, in milliseconds
};

// --- Image helpers (pure, frontend-agnostic) --------------------------------
namespace image {

// Returns the lowercase format name inferred from the leading magic bytes
// ("png", "bmp", "jpg", "gif"), or empty when unrecognised.
std::string sniffFormat(const std::vector<uint8_t>& data);

// The conventional file extension for a format name (e.g. "png" -> ".png").
std::string fileExtension(const std::string& format);

} // namespace image

// --- Error model helpers (pure, frontend-agnostic) --------------------------
// Free helpers so any frontend can render a structured, consistent error
// presentation from a Result without duplicating the mapping logic.
namespace error {

// A short, stable, machine-friendly label for an error code (e.g. "ConnectionFailed").
const char* codeLabel(ErrorCode code);

// A short label for a severity (e.g. "ERROR").
const char* severityLabel(Severity severity);

// A single-line, user-facing summary combining severity, code and message,
// e.g. "[ERROR] ConnectionFailed: Failed to connect: timed out".
std::string format(const Result& result);

} // namespace error

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

    // Requests that an in-progress blocking read (from execute/captureImage on
    // another thread) abort as soon as possible. Safe to call at any time and
    // from a different thread than the one performing the read.
    void cancel();

    // --- VISA-specific capabilities (no-ops / errors for other protocols) -----

    // Sets the VISA I/O timeout in milliseconds.
    Result setTimeout(unsigned int timeoutMs);

    // Clears the instrument's I/O buffers (VISA device clear).
    Result clearDevice();

    // --- Image capture --------------------------------------------------------

    // Sends `request.command` and reads back a binary image (IEEE 488.2 block).
    // On success `out` is populated with the image bytes and detected format,
    // and the returned Result has success == true. On failure the Result
    // carries a structured ErrorCode (BinaryReadFailed / MalformedBlock /
    // UnsupportedImageFormat / ResponseTooLarge / NotConnected).
    Result captureImage(const ImageRequest& request, ImageCapture& out);

    // Writes raw image bytes to `path`. Returns a structured Result on failure.
    Result saveImage(const std::vector<uint8_t>& data, const std::string& path);

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
