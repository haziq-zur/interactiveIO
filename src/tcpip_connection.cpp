#include "tcpip_connection.h"
#include <iostream>
#include <ws2tcpip.h>
#include <windows.h>
#pragma comment(lib, "ws2_32.lib")

TCPIPConnection::TCPIPConnection()
    : socket_(INVALID_SOCKET)
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
    if (wsaInitialized_) {
        WSACleanup();
        wsaInitialized_ = false;
    }
}

bool TCPIPConnection::initialize()
{
    if (wsaInitialized_) {
        return true;
    }

    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        setLastError("WSAStartup failed");
        return false;
    }

    wsaInitialized_ = true;
    return true;
}

bool TCPIPConnection::connect(const std::string& address, int port)
{
    if (!wsaInitialized_ && !initialize()) {
        return false;
    }

    if (connected_) {
        disconnect();
    }

    // Create socket
    socket_ = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (socket_ == INVALID_SOCKET) {
        setLastError("Socket creation failed");
        return false;
    }

    // Setup address structure
    sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(static_cast<u_short>(port));

    // Convert std::string to std::wstring for InetPtonW
    std::wstring wide_ip(address.begin(), address.end());

    // Use InetPton instead of inet_pton
    if (InetPtonW(AF_INET, wide_ip.c_str(), &addr.sin_addr) <= 0) {
        setLastError("Invalid IP address format");
        closesocket(socket_);
        socket_ = INVALID_SOCKET;
        return false;
    }

    // Connect to instrument
    if (::connect(socket_, (sockaddr*)&addr, sizeof(addr)) == SOCKET_ERROR) {
        setLastError("Connection failed");
        closesocket(socket_);
        socket_ = INVALID_SOCKET;
        return false;
    }

    ip_ = address;
    port_ = port;
    connected_ = true;
    lastError_ = "";
    return true;
}

void TCPIPConnection::disconnect()
{
    if (socket_ != INVALID_SOCKET) {
        closesocket(socket_);
        socket_ = INVALID_SOCKET;
    }
    connected_ = false;
}

bool TCPIPConnection::sendCommand(const std::string& command)
{
    if (!connected_ || socket_ == INVALID_SOCKET) {
        setLastError("Not connected");
        return false;
    }

    std::string cmd = TCPIPUtils::ensureNewline(command);

    int sent = send(socket_, cmd.c_str(), static_cast<int>(cmd.size()), 0);
    if (sent == SOCKET_ERROR) {
        setLastError("Send failed");
        return false;
    }

    return true;
}

std::string TCPIPConnection::readResponse(int timeoutSeconds)
{
    if (!connected_ || socket_ == INVALID_SOCKET) {
        setLastError("Not connected");
        return "";
    }

    // Use select to check for available data with timeout
    fd_set readfds;
    FD_ZERO(&readfds);
    FD_SET(socket_, &readfds);

    timeval timeout;
    timeout.tv_sec = timeoutSeconds;
    timeout.tv_usec = 0;

    int selectResult = select(0, &readfds, NULL, NULL, &timeout);

    if (selectResult > 0 && FD_ISSET(socket_, &readfds)) {
        char buffer[1024] = { 0 };
        int bytesReceived = recv(socket_, buffer, sizeof(buffer) - 1, 0);
        if (bytesReceived > 0) {
            buffer[bytesReceived] = '\0';
            return std::string(buffer);
        }
        else if (bytesReceived == 0) {
            setLastError("Connection closed");
            connected_ = false;
        }
        else {
            setLastError("Receive error");
        }
    }
    else if (selectResult == 0) {
        setLastError("Timeout waiting for response");
    }
    else {
        setLastError("Error in select()");
    }

    return "";
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
    // TCP/IP is always available on Windows
    return true;
}

void TCPIPConnection::setLastError(const std::string& error)
{
    lastError_ = error;
}

// Utility functions implementation
namespace TCPIPUtils {

bool isValidIPAddress(const std::string& ip)
{
    if (ip.empty()) {
        return false;
    }

    // Convert to wide string for InetPtonW
    std::wstring wide_ip(ip.begin(), ip.end());
    struct in_addr addr;
    
    return InetPtonW(AF_INET, wide_ip.c_str(), &addr) == 1;
}

std::string ensureNewline(const std::string& command)
{
    std::string result = command;
    if (!result.empty() && result.back() != '\n') {
        result += '\n';
    }
    return result;
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
