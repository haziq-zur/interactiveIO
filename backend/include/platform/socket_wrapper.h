#ifndef SOCKET_WRAPPER_H
#define SOCKET_WRAPPER_H

#include "platform_config.h"
#include <string>
#include <cstdint>

// Platform-specific includes (must be outside namespace)
#ifdef PLATFORM_WINDOWS
    #include <winsock2.h>
    #include <ws2tcpip.h>
#else
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <arpa/inet.h>
    #include <unistd.h>
    #include <fcntl.h>
    #include <errno.h>
#endif

namespace Platform {

// Platform-agnostic socket type
#ifdef PLATFORM_WINDOWS
    typedef SOCKET SocketHandle;
    #define INVALID_SOCKET_HANDLE INVALID_SOCKET
    #define SOCKET_ERROR_CODE WSAGetLastError()
#else
    typedef int SocketHandle;
    #define INVALID_SOCKET_HANDLE (-1)
    #define SOCKET_ERROR_CODE errno
#endif

// Socket initialization/cleanup
class SocketInitializer {
public:
    SocketInitializer();
    ~SocketInitializer();
    bool isInitialized() const { return m_initialized; }
    std::string getLastError() const;

private:
    bool m_initialized;
};

// Socket operations
class Socket {
public:
    Socket();
    ~Socket();

    // Core operations
    bool create();
    bool connect(const std::string& host, uint16_t port, int timeoutMs = 5000);
    bool send(const void* data, size_t length);
    bool receive(void* buffer, size_t length, size_t& bytesReceived);
    bool close();

    // Status
    bool isValid() const;
    bool isConnected() const;
    std::string getLastError() const;

    // Options
    bool setNonBlocking(bool nonBlocking);
    bool setTimeout(int timeoutMs);

private:
    SocketHandle m_socket;
    bool m_connected;
    std::string m_lastError;

    void setError(const std::string& prefix);
};

} // namespace Platform

#endif // SOCKET_WRAPPER_H
