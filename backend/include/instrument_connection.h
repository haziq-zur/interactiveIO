#ifndef INSTRUMENT_CONNECTION_H
#define INSTRUMENT_CONNECTION_H

#include <cstdint>
#include <string>
#include <vector>

/**
 * @brief Abstract base class for instrument connections
 * 
 * Defines the interface for all connection types (TCP/IP, VISA, etc.)
 * allowing runtime protocol selection.
 */
class IInstrumentConnection {
public:
    virtual ~IInstrumentConnection() = default;

    /**
     * @brief Initialize the connection system
     * @return true if initialization successful, false otherwise
     */
    virtual bool initialize() = 0;

    /**
     * @brief Connect to the instrument
     * @param address Connection address (IP, resource string, etc.)
     * @param port Port number (for TCP/IP, ignored for VISA)
     * @return true if connection successful, false otherwise
     */
    virtual bool connect(const std::string& address, int port = 0) = 0;

    /**
     * @brief Disconnect from the instrument
     */
    virtual void disconnect() = 0;

    /**
     * @brief Send a command to the instrument
     * @param command The command to send
     * @return true if successful, false otherwise
     */
    virtual bool sendCommand(const std::string& command) = 0;

    /**
     * @brief Read response from the instrument
     * @param timeoutSeconds Timeout in seconds
     * @return Response string (empty if timeout or error)
     */
    virtual std::string readResponse(int timeoutSeconds = 10) = 0;

    /**
     * @brief Read a raw binary response from the instrument
     *
     * Unlike readResponse(), this performs no text processing: it returns the
     * exact bytes received, which may include NUL and newline characters (as in
     * an IEEE 488.2 binary block / screen capture). When the response is a
     * definite-length block the read continues until the full payload has
     * arrived. Reading stops if `maxBytes` is reached to bound memory use.
     *
     * @param timeoutSeconds Timeout in seconds
     * @param maxBytes Maximum number of bytes to accumulate (safety cap)
     * @return Raw bytes received (empty on timeout or error)
     */
    virtual std::vector<uint8_t> readBinaryResponse(int timeoutSeconds, size_t maxBytes) = 0;

    /**
     * @brief Send query and read response (convenience method)
     * @param command Query command (should contain '?')
     * @param timeoutSeconds Timeout in seconds
     * @return Response string
     */
    virtual std::string query(const std::string& command, int timeoutSeconds = 10) {
        if (sendCommand(command)) {
            return readResponse(timeoutSeconds);
        }
        return "";
    }

    /**
     * @brief Check if connected to instrument
     * @return true if connected, false otherwise
     */
    virtual bool isConnected() const = 0;

    /**
     * @brief Get the last error message
     * @return Error message string
     */
    virtual std::string getLastError() const = 0;

    /**
     * @brief Get connection type name
     * @return Connection type (e.g., "TCP/IP", "VISA")
     */
    virtual std::string getConnectionType() const = 0;

    /**
     * @brief Get connection information
     * @return Connection details string
     */
    virtual std::string getConnectionInfo() const = 0;

    /**
     * @brief Check if this connection type is available
     * @return true if the underlying library/driver is available
     */
    virtual bool isAvailable() const = 0;
};

#endif // INSTRUMENT_CONNECTION_H
