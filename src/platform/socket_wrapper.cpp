#include "platform/socket_wrapper.h"
#include <sstream>
#include <cstring>

#ifdef PLATFORM_WINDOWS
    #define WIN32_LEAN_AND_MEAN
    #pragma comment(lib, "ws2_32.lib")
#endif

// Define wrapper functions after headers are included
#ifdef PLATFORM_WINDOWS
    // Wrapper functions to avoid name conflicts with class methods
    static inline int connect_socket(SOCKET s, const struct sockaddr* name, int namelen) {
        return connect(s, name, namelen);
    }
    static inline int send_socket(SOCKET s, const char* buf, int len, int flags) {
        return send(s, buf, len, flags);
    }
    static inline int recv_socket(SOCKET s, char* buf, int len, int flags) {
        return recv(s, buf, len, flags);
    }
#endif

namespace Platform {

// ============================================================================
// SocketInitializer Implementation
// ============================================================================

SocketInitializer::SocketInitializer() : m_initialized(false) {
#ifdef PLATFORM_WINDOWS
    WSADATA wsaData;
    int result = WSAStartup(MAKEWORD(2, 2), &wsaData);
    m_initialized = (result == 0);
#else
    // No initialization needed on Unix systems
    m_initialized = true;
#endif
}

SocketInitializer::~SocketInitializer() {
#ifdef PLATFORM_WINDOWS
    if (m_initialized) {
        WSACleanup();
    }
#endif
}

std::string SocketInitializer::getLastError() const {
#ifdef PLATFORM_WINDOWS
    return "WSAStartup failed";
#else
    return "Socket initialization error";
#endif
}

// ============================================================================
// Socket Implementation
// ============================================================================

Socket::Socket() 
    : m_socket(INVALID_SOCKET_HANDLE)
    , m_connected(false) 
{
}

Socket::~Socket() {
    close();
}

bool Socket::create() {
    m_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (m_socket == INVALID_SOCKET_HANDLE) {
        setError("Failed to create socket");
        return false;
    }
    return true;
}

bool Socket::connect(const std::string& host, uint16_t port, int timeoutMs) {
    if (m_socket == INVALID_SOCKET_HANDLE) {
        if (!create()) {
            return false;
        }
    }

    // Parse IP address
    struct sockaddr_in serverAddr;
    std::memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(port);

#ifdef PLATFORM_WINDOWS
    // Windows: Use InetPtonA (ASCII version)
    if (InetPtonA(AF_INET, host.c_str(), &serverAddr.sin_addr) != 1) {
        setError("Invalid IP address format");
        return false;
    }
#else
    // Unix: Use inet_pton
    if (inet_pton(AF_INET, host.c_str(), &serverAddr.sin_addr) != 1) {
        setError("Invalid IP address format");
        return false;
    }
#endif

    // Set non-blocking for timeout
    if (!setNonBlocking(true)) {
        return false;
    }

    // Attempt connection (use wrapper function to avoid member method conflict)
    int connectResult;
#ifdef PLATFORM_WINDOWS
    connectResult = connect_socket(m_socket, (const struct sockaddr*)&serverAddr, sizeof(serverAddr));
#else
    connectResult = ::connect(m_socket, (struct sockaddr*)&serverAddr, sizeof(serverAddr));
#endif
    
#ifdef PLATFORM_WINDOWS
    bool inProgress = (WSAGetLastError() == WSAEWOULDBLOCK);
#else
    bool inProgress = (errno == EINPROGRESS);
#endif

    if (connectResult == 0) {
        // Connected immediately
        m_connected = true;
        setNonBlocking(false);
        return true;
    } else if (!inProgress) {
        setError("Connection failed");
        return false;
    }

    // Wait for connection with timeout
    fd_set writeSet;
    FD_ZERO(&writeSet);
    FD_SET(m_socket, &writeSet);

    struct timeval timeout;
    timeout.tv_sec = timeoutMs / 1000;
    timeout.tv_usec = (timeoutMs % 1000) * 1000;

    int selectResult = select(static_cast<int>(m_socket) + 1, nullptr, &writeSet, nullptr, &timeout);
    
    if (selectResult <= 0) {
        setError("Connection timeout");
        close();
        return false;
    }

    // Check if connection succeeded
    int error = 0;
    socklen_t len = sizeof(error);
#ifdef PLATFORM_WINDOWS
    if (getsockopt(m_socket, SOL_SOCKET, SO_ERROR, (char*)&error, &len) < 0 || error != 0) {
#else
    if (getsockopt(m_socket, SOL_SOCKET, SO_ERROR, &error, &len) < 0 || error != 0) {
#endif
        setError("Connection failed");
        close();
        return false;
    }

    m_connected = true;
    setNonBlocking(false);
    return true;
}

bool Socket::send(const void* data, size_t length) {
    if (!m_connected) {
        m_lastError = "Not connected";
        return false;
    }

    size_t totalSent = 0;
    const char* buffer = static_cast<const char*>(data);

    while (totalSent < length) {
        int bytesSent;
#ifdef PLATFORM_WINDOWS
        bytesSent = send_socket(m_socket, buffer + totalSent, static_cast<int>(length - totalSent), 0);
#else
        bytesSent = ::send(m_socket, buffer + totalSent, length - totalSent, 0);
#endif
        if (bytesSent <= 0) {
            setError("Send failed");
            return false;
        }
        totalSent += static_cast<size_t>(bytesSent);
    }

    return true;
}

bool Socket::receive(void* buffer, size_t length, size_t& bytesReceived) {
    if (!m_connected) {
        m_lastError = "Not connected";
        return false;
    }

#ifdef PLATFORM_WINDOWS
    int received = recv_socket(m_socket, static_cast<char*>(buffer), static_cast<int>(length), 0);
#else
    ssize_t received = recv(m_socket, static_cast<char*>(buffer), length, 0);
#endif
    
    if (received < 0) {
        setError("Receive failed");
        bytesReceived = 0;
        return false;
    }

    bytesReceived = static_cast<size_t>(received);
    return true;
}

bool Socket::close() {
    if (m_socket == INVALID_SOCKET_HANDLE) {
        return true;
    }

#ifdef PLATFORM_WINDOWS
    closesocket(m_socket);
#else
    ::close(m_socket);
#endif

    m_socket = INVALID_SOCKET_HANDLE;
    m_connected = false;
    return true;
}

bool Socket::isValid() const {
    return m_socket != INVALID_SOCKET_HANDLE;
}

bool Socket::isConnected() const {
    return m_connected;
}

std::string Socket::getLastError() const {
    return m_lastError;
}

bool Socket::setNonBlocking(bool nonBlocking) {
#ifdef PLATFORM_WINDOWS
    u_long mode = nonBlocking ? 1 : 0;
    if (ioctlsocket(m_socket, FIONBIO, &mode) != 0) {
        setError("Failed to set non-blocking mode");
        return false;
    }
#else
    int flags = fcntl(m_socket, F_GETFL, 0);
    if (flags < 0) {
        setError("Failed to get socket flags");
        return false;
    }
    
    flags = nonBlocking ? (flags | O_NONBLOCK) : (flags & ~O_NONBLOCK);
    
    if (fcntl(m_socket, F_SETFL, flags) < 0) {
        setError("Failed to set non-blocking mode");
        return false;
    }
#endif
    return true;
}

bool Socket::setTimeout(int timeoutMs) {
#ifdef PLATFORM_WINDOWS
    DWORD timeout = static_cast<DWORD>(timeoutMs);
    if (setsockopt(m_socket, SOL_SOCKET, SO_RCVTIMEO, 
                   (const char*)&timeout, sizeof(timeout)) != 0) {
        setError("Failed to set timeout");
        return false;
    }
#else
    struct timeval timeout;
    timeout.tv_sec = timeoutMs / 1000;
    timeout.tv_usec = (timeoutMs % 1000) * 1000;
    
    if (setsockopt(m_socket, SOL_SOCKET, SO_RCVTIMEO, 
                   &timeout, sizeof(timeout)) < 0) {
        setError("Failed to set timeout");
        return false;
    }
#endif
    return true;
}

void Socket::setError(const std::string& prefix) {
    std::ostringstream oss;
    oss << prefix << ": ";
    
#ifdef PLATFORM_WINDOWS
    int errorCode = WSAGetLastError();
    char* message = nullptr;
    FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
                   nullptr, errorCode, 0, (char*)&message, 0, nullptr);
    if (message) {
        oss << message;
        LocalFree(message);
    } else {
        oss << "Error code " << errorCode;
    }
#else
    oss << strerror(errno);
#endif
    
    m_lastError = oss.str();
}

} // namespace Platform
