#include "tcpip_connection.h"
#include "platform/socket_wrapper.h"
#include "ieee4882.h"
#include <iostream>
#include <sstream>

// Global socket initializer (constructs before any sockets)
static Platform::SocketInitializer g_socketInit;

// Private implementation class
class TCPIPConnection::Impl {
public:
    Platform::Socket socket;
    std::string ip;
    uint16_t port;
    std::string lastError;
    bool connected;

    Impl() : port(0), connected(false) {}
};

TCPIPConnection::TCPIPConnection()
    : pImpl_(new Impl())
    , socket_(0)  // Legacy member, kept for compatibility
    , ip_("")
    , port_(0)
    , connected_(false)
    , wsaInitialized_(false)
    , lastError_("")
{
}

TCPIPConnection::~TCPIPConnection()
{
    disconnect();
    delete pImpl_;
}

bool TCPIPConnection::initialize()
{
    if (!g_socketInit.isInitialized()) {
        setLastError(g_socketInit.getLastError());
        return false;
    }
    wsaInitialized_ = true;
    return true;
}

bool TCPIPConnection::connect(const std::string& address, int port)
{
    if (!g_socketInit.isInitialized() && !initialize()) {
        return false;
    }

    if (pImpl_->connected) {
        disconnect();
    }

    // Connect using platform wrapper
    if (!pImpl_->socket.connect(address, static_cast<uint16_t>(port), 5000)) {
        setLastError(pImpl_->socket.getLastError());
        return false;
    }

    pImpl_->ip = address;
    pImpl_->port = static_cast<uint16_t>(port);
    pImpl_->connected = true;
    
    // Update legacy members for compatibility
    ip_ = address;
    port_ = port;
    connected_ = true;
    lastError_ = "";
    
    return true;
}

void TCPIPConnection::disconnect()
{
    if (pImpl_->connected) {
        pImpl_->socket.close();
        pImpl_->connected = false;
    }
    connected_ = false;
}

bool TCPIPConnection::sendCommand(const std::string& command)
{
    if (!pImpl_->connected) {
        setLastError("Not connected");
        return false;
    }

    std::string cmd = TCPIPUtils::ensureNewline(command);

    if (!pImpl_->socket.send(cmd.c_str(), cmd.size())) {
        setLastError(pImpl_->socket.getLastError());
        return false;
    }

    return true;
}

std::string TCPIPConnection::readResponse(int timeoutSeconds)
{
    if (!pImpl_->connected) {
        setLastError("Not connected");
        return "";
    }

    // Set timeout
    pImpl_->socket.setTimeout(timeoutSeconds * 1000);

    // Read data
    char buffer[4096] = { 0 };
    size_t bytesReceived = 0;
    
    if (pImpl_->socket.receive(buffer, sizeof(buffer) - 1, bytesReceived)) {
        if (bytesReceived > 0) {
            buffer[bytesReceived] = '\0';
            return std::string(buffer);
        } else if (bytesReceived == 0) {
            setLastError("Connection closed");
            pImpl_->connected = false;
            connected_ = false;
        }
    } else {
        std::string error = pImpl_->socket.getLastError();
        // Check if it's just a timeout
        if (error.find("timeout") != std::string::npos || 
            error.find("Timeout") != std::string::npos) {
            setLastError("Timeout waiting for response");
        } else {
            setLastError(error);
        }
    }

    return "";
}

std::vector<uint8_t> TCPIPConnection::readBinaryResponse(int timeoutSeconds, size_t maxBytes)
{
    std::vector<uint8_t> buffer;

    if (!pImpl_->connected) {
        setLastError("Not connected");
        return buffer;
    }

    pImpl_->socket.setTimeout(timeoutSeconds * 1000);

    // Read raw chunks until the full IEEE 488.2 block has arrived, the size cap
    // is hit, or the peer times out / closes. `needed` is the total block size
    // (header + payload) once it can be determined from the header.
    char chunk[65536];
    size_t needed = 0;

    while (true) {
        size_t got = 0;
        if (!pImpl_->socket.receive(chunk, sizeof(chunk), got)) {
            std::string error = pImpl_->socket.getLastError();
            if (error.find("timeout") != std::string::npos
                || error.find("Timeout") != std::string::npos) {
                setLastError("Timeout waiting for response");
            } else {
                setLastError(error);
            }
            break;
        }
        if (got == 0) {
            setLastError("Connection closed");
            pImpl_->connected = false;
            connected_ = false;
            break;
        }

        buffer.insert(buffer.end(), chunk, chunk + got);

        if (needed == 0) {
            ieee4882::BlockHeader header;
            std::string error;
            if (!ieee4882::parseHeader(buffer.data(), buffer.size(), header, error)) {
                // Not a binary block (e.g. a plain text error response). Return
                // what we have and let the caller decide.
                break;
            }
            if (header.headerComplete && !header.indefinite) {
                needed = header.headerLength + header.payloadLength;
                if (needed > maxBytes) {
                    setLastError("Binary response exceeds maximum size ("
                                 + std::to_string(maxBytes) + " bytes)");
                    return buffer;
                }
            } else if (header.headerComplete && header.indefinite) {
                // Indefinite block: read until the trailing newline terminator.
                if (!buffer.empty() && buffer.back() == '\n') {
                    break;
                }
            }
        }

        if (needed > 0 && buffer.size() >= needed) {
            break;
        }
        if (buffer.size() >= maxBytes) {
            setLastError("Binary response exceeds maximum size ("
                         + std::to_string(maxBytes) + " bytes)");
            break;
        }
    }

    return buffer;
}

bool TCPIPConnection::isConnected() const
{
    return connected_;
}

std::string TCPIPConnection::getLastError() const
{
    return lastError_;
}

std::string TCPIPConnection::getConnectionType() const
{
    return "TCP/IP Socket";
}

std::string TCPIPConnection::getConnectionInfo() const
{
    if (connected_) {
        return "TCP/IP: " + ip_ + ":" + std::to_string(port_);
    }
    return "TCP/IP: Not connected";
}

bool TCPIPConnection::isAvailable() const
{
    // TCP/IP is available if socket system is initialized (sockets are built into the OS)
    return g_socketInit.isInitialized();
}

void TCPIPConnection::setLastError(const std::string& error)
{
    lastError_ = error;
    pImpl_->lastError = error;
}

// Utility functions implementation
namespace TCPIPUtils {

std::string ensureNewline(const std::string& command)
{
    std::string result = command;
    if (!result.empty() && result.back() != '\n') {
        result += '\n';
    }
    return result;
}

bool isValidIPAddress(const std::string& ip)
{
    if (ip.empty()) {
        return false;
    }

    // Parse and validate each octet
    std::istringstream iss(ip);
    std::string octet;
    int count = 0;
    
    while (std::getline(iss, octet, '.')) {
        count++;
        
        // Check if octet is empty or has invalid characters
        if (octet.empty() || octet.length() > 3) {
            return false;
        }
        
        // Check all characters are digits
        for (char c : octet) {
            if (!std::isdigit(static_cast<unsigned char>(c))) {
                return false;
            }
        }
        
        // Check numeric value is in range 0-255
        int value = std::stoi(octet);
        if (value < 0 || value > 255) {
            return false;
        }
        
        // Check for leading zeros (except for "0" itself)
        if (octet.length() > 1 && octet[0] == '0') {
            return false;
        }
    }
    
    // Must have exactly 4 octets
    return count == 4;
}

bool isQueryCommand(const std::string& command)
{
    // A query command contains '?' and typically the '?' should be near the end
    size_t pos = command.find('?');
    if (pos == std::string::npos) {
        return false;
    }
    
    // Check if '?' is followed only by whitespace or newline
    for (size_t i = pos + 1; i < command.length(); ++i) {
        char c = command[i];
        if (c != ' ' && c != '\t' && c != '\r' && c != '\n') {
            return false;
        }
    }
    
    return true;
}

} // namespace TCPIPUtils
