#include "instrument_controller.h"

#include "tcpip_connection.h"
#include "visa_connection.h"

#include <chrono>

namespace iio {

namespace {

// Fills a Result as a failure with a structured code + severity. Centralizing
// this keeps every error path consistent and easy to audit.
Result makeError(ErrorCode code, Severity severity, std::string message)
{
    Result result;
    result.success = false;
    result.code = code;
    result.severity = severity;
    result.message = std::move(message);
    return result;
}

} // namespace

InstrumentController::InstrumentController() = default;

InstrumentController::~InstrumentController()
{
    if (connection_ && connection_->isConnected()) {
        connection_->disconnect();
    }
}

std::unique_ptr<IInstrumentConnection>
InstrumentController::createConnection(Protocol protocol)
{
    switch (protocol) {
        case Protocol::TCPIP:
            return std::make_unique<TCPIPConnection>();
        case Protocol::VISA:
            return std::make_unique<VISAConnection>();
    }
    return nullptr;
}

Result InstrumentController::connect(const ConnectionConfig& config)
{
    Result result;

    // Tear down any previous connection first.
    if (connection_) {
        if (connection_->isConnected()) {
            connection_->disconnect();
        }
        connection_.reset();
    }

    connection_ = createConnection(config.protocol);
    if (!connection_) {
        lastError_ = "Unsupported protocol";
        return makeError(ErrorCode::UnsupportedProtocol, Severity::Error, lastError_);
    }
    currentProtocol_ = config.protocol;

    // For VISA, verify the runtime is present before attempting to use it.
    if (config.protocol == Protocol::VISA && !connection_->isAvailable()) {
        lastError_ = "VISA library not found. Please install NI-VISA from "
                     "https://www.ni.com/visa";
        connection_.reset();
        return makeError(ErrorCode::ProtocolUnavailable, Severity::Error, lastError_);
    }

    if (!connection_->initialize()) {
        lastError_ = connection_->getLastError();
        connection_.reset();
        return makeError(ErrorCode::InitializationFailed, Severity::Error,
                         "Failed to initialize: " + lastError_);
    }

    bool connected = false;
    if (config.protocol == Protocol::TCPIP) {
        connected = connection_->connect(config.address, config.port);
    } else {
        connected = connection_->connect(config.address);
    }

    if (!connected) {
        lastError_ = connection_->getLastError();
        connection_.reset();
        return makeError(ErrorCode::ConnectionFailed, Severity::Error,
                         "Failed to connect: " + lastError_);
    }

    result.success = true;
    result.message = "Connected successfully (" + connection_->getConnectionType() + ")";
    return result;
}

Result InstrumentController::disconnect()
{
    Result result;
    if (connection_ && connection_->isConnected()) {
        connection_->disconnect();
    }
    connection_.reset();
    result.success = true;
    result.message = "Disconnected";
    return result;
}

bool InstrumentController::isConnected() const
{
    return connection_ && connection_->isConnected();
}

bool InstrumentController::isProtocolAvailable(Protocol protocol) const
{
    // A connected controller can report its own connection's availability;
    // otherwise create a throwaway instance to probe the runtime.
    if (connection_ && protocol == currentProtocol_) {
        return connection_->isAvailable();
    }
    auto probe = createConnection(protocol);
    return probe && probe->isAvailable();
}

Result InstrumentController::execute(const std::string& command, int timeoutSeconds)
{
    Result result;

    if (!isConnected()) {
        lastError_ = "Not connected";
        return makeError(ErrorCode::NotConnected, Severity::Error, lastError_);
    }

    const bool isQuery = command.find('?') != std::string::npos;
    const int effectiveTimeout =
        (timeoutSeconds < 0) ? settings_.responseTimeoutSeconds : timeoutSeconds;

    // Append the configured end-of-line sequence before sending.
    const std::string payload = command + settings_.eol;

    // Measure the full round trip: from just before the command is sent until
    // the response (or send acknowledgement) is received.
    const auto startTime = std::chrono::steady_clock::now();
    const auto elapsedMs = [&startTime]() {
        const auto end = std::chrono::steady_clock::now();
        return std::chrono::duration<double, std::milli>(end - startTime).count();
    };

    if (!connection_->sendCommand(payload)) {
        lastError_ = connection_->getLastError();
        result = makeError(ErrorCode::SendFailed, Severity::Error,
                           "Failed to send command: " + lastError_);
        result.elapsedMs = elapsedMs();
        return result;
    }

    if (!isQuery) {
        result.success = true;
        result.message = "Command sent successfully";
        result.elapsedMs = elapsedMs();
        return result;
    }

    const std::string response = connection_->readResponse(effectiveTimeout);
    if (response.empty()) {
        lastError_ = connection_->getLastError();
        // A query that yields no data is reported as a non-fatal warning so the
        // frontend can surface the timeout, while the connection stays open.
        result = makeError(ErrorCode::NoResponse, Severity::Warning,
                           lastError_.empty()
                               ? "No response received (timeout or error)"
                               : "No response received: " + lastError_);
        result.elapsedMs = elapsedMs();
        return result;
    }

    result.success = true;
    result.hasResponse = true;
    result.response = response;
    result.message = "Response received";
    result.elapsedMs = elapsedMs();
    return result;
}

Result InstrumentController::setTimeout(unsigned int timeoutMs)
{
    Result result;
    auto* visa = dynamic_cast<VISAConnection*>(connection_.get());
    if (!visa) {
        return makeError(ErrorCode::OperationUnsupported, Severity::Warning,
                         "Timeout is only configurable for VISA connections");
    }
    visa->setTimeout(timeoutMs);
    result.success = true;
    result.message = "Timeout set to " + std::to_string(timeoutMs) + " ms";
    return result;
}

Result InstrumentController::clearDevice()
{
    Result result;
    auto* visa = dynamic_cast<VISAConnection*>(connection_.get());
    if (!visa) {
        return makeError(ErrorCode::OperationUnsupported, Severity::Warning,
                         "Device clear is only available for VISA connections");
    }
    if (!visa->clear()) {
        lastError_ = visa->getLastError();
        return makeError(ErrorCode::ClearFailed, Severity::Error,
                         "Clear failed: " + lastError_);
    }
    result.success = true;
    result.message = "Instrument buffer cleared";
    return result;
}

void InstrumentController::setCommunicationSettings(const CommunicationSettings& settings)
{
    settings_ = settings;

    // Keep the VISA I/O timeout in sync with the response timeout when a VISA
    // connection is active (no-op for other transports).
    if (auto* visa = dynamic_cast<VISAConnection*>(connection_.get())) {
        visa->setTimeout(static_cast<unsigned int>(settings_.responseTimeoutSeconds) * 1000u);
    }
}

const CommunicationSettings& InstrumentController::communicationSettings() const
{
    return settings_;
}

std::string InstrumentController::connectionInfo() const
{
    return connection_ ? connection_->getConnectionInfo() : std::string();
}

std::string InstrumentController::connectionType() const
{
    return connection_ ? connection_->getConnectionType() : std::string();
}

Protocol InstrumentController::currentProtocol() const
{
    return currentProtocol_;
}

std::string InstrumentController::lastError() const
{
    return lastError_;
}

namespace error {

const char* codeLabel(ErrorCode code)
{
    switch (code) {
        case ErrorCode::None:                 return "None";
        case ErrorCode::NotConnected:         return "NotConnected";
        case ErrorCode::UnsupportedProtocol:  return "UnsupportedProtocol";
        case ErrorCode::ProtocolUnavailable:  return "ProtocolUnavailable";
        case ErrorCode::InitializationFailed: return "InitializationFailed";
        case ErrorCode::ConnectionFailed:     return "ConnectionFailed";
        case ErrorCode::SendFailed:           return "SendFailed";
        case ErrorCode::NoResponse:           return "NoResponse";
        case ErrorCode::OperationUnsupported: return "OperationUnsupported";
        case ErrorCode::ClearFailed:          return "ClearFailed";
        case ErrorCode::InvalidInput:         return "InvalidInput";
    }
    return "Unknown";
}

const char* severityLabel(Severity severity)
{
    switch (severity) {
        case Severity::Info:    return "INFO";
        case Severity::Warning: return "WARNING";
        case Severity::Error:   return "ERROR";
    }
    return "INFO";
}

std::string format(const Result& result)
{
    std::string out = "[";
    out += severityLabel(result.severity);
    out += "] ";
    if (result.code != ErrorCode::None) {
        out += codeLabel(result.code);
        out += ": ";
    }
    out += result.message;
    return out;
}

} // namespace error

namespace resource {

bool isValidIPAddress(const std::string& ip)
{
    return TCPIPUtils::isValidIPAddress(ip);
}

bool isValidResourceString(const std::string& resourceString)
{
    return VISAUtils::isValidResourceString(resourceString);
}

std::string buildTCPIPResource(const std::string& ip)
{
    return VISAUtils::buildTCPIPResource(ip);
}

std::string buildSocketResource(const std::string& ip, int port)
{
    return VISAUtils::buildTCPIPResource(ip, port);
}

std::string buildGPIBResource(int board, int address)
{
    return VISAUtils::buildGPIBResource(board, address);
}

std::string interfaceType(const std::string& resourceString)
{
    return VISAUtils::getInterfaceType(resourceString);
}

} // namespace resource

} // namespace iio
