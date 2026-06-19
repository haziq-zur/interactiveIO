#include "instrument_controller.h"

#include "tcpip_connection.h"
#include "visa_connection.h"

namespace iio {

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
        result.message = lastError_;
        return result;
    }
    currentProtocol_ = config.protocol;

    // For VISA, verify the runtime is present before attempting to use it.
    if (config.protocol == Protocol::VISA && !connection_->isAvailable()) {
        lastError_ = "VISA library not found. Please install NI-VISA from "
                     "https://www.ni.com/visa";
        result.message = lastError_;
        connection_.reset();
        return result;
    }

    if (!connection_->initialize()) {
        lastError_ = connection_->getLastError();
        result.message = "Failed to initialize: " + lastError_;
        connection_.reset();
        return result;
    }

    bool connected = false;
    if (config.protocol == Protocol::TCPIP) {
        connected = connection_->connect(config.address, config.port);
    } else {
        connected = connection_->connect(config.address);
    }

    if (!connected) {
        lastError_ = connection_->getLastError();
        result.message = "Failed to connect: " + lastError_;
        connection_.reset();
        return result;
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
        result.message = lastError_;
        return result;
    }

    const bool isQuery = command.find('?') != std::string::npos;

    if (!connection_->sendCommand(command)) {
        lastError_ = connection_->getLastError();
        result.message = "Failed to send command: " + lastError_;
        return result;
    }

    if (!isQuery) {
        result.success = true;
        result.message = "Command sent successfully";
        return result;
    }

    const std::string response = connection_->readResponse(timeoutSeconds);
    if (response.empty()) {
        lastError_ = connection_->getLastError();
        result.message = lastError_.empty()
            ? "No response received (timeout or error)"
            : "No response received: " + lastError_;
        // A query that yields no data is reported as a non-fatal failure so the
        // frontend can surface the timeout, while the connection stays open.
        return result;
    }

    result.success = true;
    result.hasResponse = true;
    result.response = response;
    result.message = "Response received";
    return result;
}

Result InstrumentController::setTimeout(unsigned int timeoutMs)
{
    Result result;
    auto* visa = dynamic_cast<VISAConnection*>(connection_.get());
    if (!visa) {
        result.message = "Timeout is only configurable for VISA connections";
        return result;
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
        result.message = "Device clear is only available for VISA connections";
        return result;
    }
    if (!visa->clear()) {
        lastError_ = visa->getLastError();
        result.message = "Clear failed: " + lastError_;
        return result;
    }
    result.success = true;
    result.message = "Instrument buffer cleared";
    return result;
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
