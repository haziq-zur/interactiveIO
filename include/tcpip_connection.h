#ifndef TCPIP_CONNECTION_H
#define TCPIP_CONNECTION_H

#include "instrument_connection.h"
#include <winsock2.h>
#include <string>

/**
 * @brief TCP/IP Socket connection implementation
 * 
 * Direct socket communication using Winsock2 for Windows.
 * Provides fast, lightweight connection to instruments over TCP/IP.
 */
class TCPIPConnection : public IInstrumentConnection {
public:
    TCPIPConnection();
    virtual ~TCPIPConnection() override;

    // IInstrumentConnection interface implementation
    virtual bool initialize() override;
    virtual bool connect(const std::string& address, int port = 5025) override;
    virtual void disconnect() override;
    virtual bool sendCommand(const std::string& command) override;
    virtual std::string readResponse(int timeoutSeconds = 10) override;
    virtual bool isConnected() const override;
    virtual std::string getLastError() const override;
    virtual std::string getConnectionType() const override;
    virtual std::string getConnectionInfo() const override;
    virtual bool isAvailable() const override;

    // TCP/IP specific methods
    std::string getIP() const { return ip_; }
    int getPort() const { return port_; }

private:
    SOCKET socket_;
    std::string ip_;
    int port_;
    bool connected_;
    bool wsaInitialized_;
    std::string lastError_;
    
    void setLastError(const std::string& error);
};

// Utility functions for TCP/IP connections
namespace TCPIPUtils {
    // Validate IP address format
    bool isValidIPAddress(const std::string& ip);
    
    // Ensure command ends with newline
    std::string ensureNewline(const std::string& command);
    
    // Check if command is a query (expects response)
    bool isQueryCommand(const std::string& command);
}

#endif // TCPIP_CONNECTION_H
