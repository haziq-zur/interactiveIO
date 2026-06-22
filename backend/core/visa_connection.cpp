#include "visa_connection.h"
#include "platform/dynamic_loader.h"
#include "ieee4882.h"
#include <iostream>
#include <sstream>
#include <string>
#include <algorithm>
#include <cctype>
#include <chrono>

#ifndef VI_SUCCESS_MAX_CNT
#define VI_SUCCESS_MAX_CNT 0x3FFF0006L
#endif

#ifndef VI_ERROR_TMO
#define VI_ERROR_TMO 0xBFFF0015L
#endif

VISAConnection::VISAConnection()
    : resourceString_("")
    , lastError_("")
    , connected_(false)
    , rmInitialized_(false)
    , visaAvailable_(false)
    , timeoutMs_(10000)  // 10 second default timeout
    , visaDll_(nullptr)
    , defaultRM_(0)
    , instrSession_(0)
    , viOpenDefaultRM_(nullptr)
    , viOpen_(nullptr)
    , viClose_(nullptr)
    , viWrite_(nullptr)
    , viRead_(nullptr)
    , viSetAttribute_(nullptr)
    , viClear_(nullptr)
    , viStatusDesc_(nullptr)
    , visaLib_(new Platform::DynamicLibrary())
{
}

VISAConnection::~VISAConnection()
{
    disconnect();
    if (rmInitialized_ && defaultRM_ && viClose_) {
        viClose_(defaultRM_);
        rmInitialized_ = false;
    }
    unloadVISALibrary();
    delete visaLib_;
}

bool VISAConnection::loadVISALibrary()
{
    if (visaDll_) {
        return true;  // Already loaded
    }

    // Try to load VISA library
#ifdef PLATFORM_WINDOWS
    std::string libName = "visa64";
#else
    std::string libName = "visa";
#endif

    if (!visaLib_->load(libName)) {
        setLastError("VISA library not found. Please install NI-VISA or compatible VISA driver. " + 
                     visaLib_->getLastError());
        return false;
    }

    // Load function pointers using platform wrapper
    bool success = true;
    success &= visaLib_->getFunction("viOpenDefaultRM", viOpenDefaultRM_);
    success &= visaLib_->getFunction("viOpen", viOpen_);
    success &= visaLib_->getFunction("viClose", viClose_);
    success &= visaLib_->getFunction("viWrite", viWrite_);
    success &= visaLib_->getFunction("viRead", viRead_);
    success &= visaLib_->getFunction("viSetAttribute", viSetAttribute_);
    success &= visaLib_->getFunction("viClear", viClear_);
    success &= visaLib_->getFunction("viStatusDesc", viStatusDesc_);

    if (!success) {
        setLastError("Failed to load VISA functions: " + visaLib_->getLastError());
        unloadVISALibrary();
        return false;
    }

    visaDll_ = (void*)1;  // Mark as loaded
    visaAvailable_ = true;
    return true;
}

void VISAConnection::unloadVISALibrary()
{
    if (visaLib_) {
        visaLib_->unload();
    }
    visaDll_ = nullptr;
    visaAvailable_ = false;
}

bool VISAConnection::initialize()
{
    if (!loadVISALibrary()) {
        return false;
    }

    if (rmInitialized_) {
        return true;
    }

    ViStatus status = viOpenDefaultRM_(&defaultRM_);
    if (status != VI_SUCCESS) {
        setLastErrorFromStatus(status);
        return false;
    }

    rmInitialized_ = true;
    return true;
}

bool VISAConnection::connect(const std::string& address, int port)
{
    if (!initialize()) {
        return false;
    }

    if (connected_) {
        disconnect();
    }

    // Build resource string if port is provided (assume TCPIP socket)
    std::string resourceStr = address;
    if (port > 0 && address.find("::") == std::string::npos) {
        resourceStr = VISAUtils::buildTCPIPResource(address, port);
    }

    // Open the instrument session
    ViStatus status = viOpen_(defaultRM_, 
                              const_cast<char*>(resourceStr.c_str()), 
                              VI_NULL, 
                              VI_NULL, 
                              &instrSession_);
    
    if (status != VI_SUCCESS) {
        setLastErrorFromStatus(status);
        return false;
    }

    // Set timeout
    if (viSetAttribute_) {
        viSetAttribute_(instrSession_, VI_ATTR_TMO_VALUE, timeoutMs_);
    }

    resourceString_ = resourceStr;
    connected_ = true;
    lastError_ = "";
    return true;
}

void VISAConnection::disconnect()
{
    if (instrSession_ && viClose_) {
        viClose_(instrSession_);
        instrSession_ = 0;
    }
    connected_ = false;
    resourceString_ = "";
}

bool VISAConnection::sendCommand(const std::string& command)
{
    if (!connected_ || !instrSession_) {
        setLastError("Not connected");
        return false;
    }

    std::string cmd = command;
    // Ensure command ends with newline
    if (!cmd.empty() && cmd.back() != '\n') {
        cmd += '\n';
    }

    ViUInt32 writeCount;
    ViStatus status = viWrite_(instrSession_, 
                               (unsigned char*)cmd.c_str(), 
                               static_cast<ViUInt32>(cmd.length()), 
                               &writeCount);
    
    if (status != VI_SUCCESS) {
        setLastErrorFromStatus(status);
        return false;
    }

    return true;
}

std::string VISAConnection::readResponse(int timeoutSeconds)
{
    if (!connected_ || !instrSession_) {
        setLastError("Not connected");
        return "";
    }

    // Clear any stale cancellation from a previous read.
    cancelRequested_ = false;

    const unsigned int oldTimeout = timeoutMs_;

    // Poll with a short per-attempt VISA timeout so a cancellation request can
    // abort the wait within ~one slice instead of blocking the whole timeout.
    const long totalMs = (timeoutSeconds > 0) ? timeoutSeconds * 1000L : 0L;
    const long sliceMs = 100;
    const auto start = std::chrono::steady_clock::now();

    const unsigned int bufferSize = 1024;
    char* buffer = new char[bufferSize];
    std::string result;

    while (true) {
        if (cancelRequested_) {
            setLastError("Request cancelled by user");
            break;
        }

        const auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                                 std::chrono::steady_clock::now() - start).count();
        const long remaining = totalMs - elapsed;
        if (remaining <= 0) {
            setLastError("Timeout waiting for response");
            break;
        }

        if (viSetAttribute_) {
            viSetAttribute_(instrSession_, VI_ATTR_TMO_VALUE,
                            static_cast<ViUInt32>(std::min(sliceMs, remaining)));
        }

        ViUInt32 readCount = 0;
        ViStatus status = viRead_(instrSession_, (unsigned char*)buffer,
                                  bufferSize - 1, &readCount);

        if (status == VI_SUCCESS || status == VI_SUCCESS_MAX_CNT || readCount > 0) {
            buffer[readCount] = '\0';
            result = std::string(buffer, readCount);
            break;
        }
        if (status == VI_ERROR_TMO) {
            continue; // slice elapsed with no data — re-check cancel / timeout
        }
        setLastErrorFromStatus(status);
        break;
    }

    delete[] buffer;

    if (viSetAttribute_) {
        viSetAttribute_(instrSession_, VI_ATTR_TMO_VALUE, oldTimeout);
    }

    return result;
}

void VISAConnection::requestCancel()
{
    cancelRequested_ = true;
}

std::vector<uint8_t> VISAConnection::readBinaryResponse(int timeoutSeconds, size_t maxBytes)
{
    std::vector<uint8_t> buffer;

    if (!connected_ || !instrSession_) {
        setLastError("Not connected");
        return buffer;
    }

    // Clear any stale cancellation from a previous read.
    cancelRequested_ = false;

    const unsigned int oldTimeout = timeoutMs_;

    // Poll with short per-attempt VISA timeouts so a cancellation request can
    // abort the wait promptly.
    const long totalMs = (timeoutSeconds > 0) ? timeoutSeconds * 1000L : 0L;
    const long sliceMs = 100;
    const auto start = std::chrono::steady_clock::now();

    // Read raw chunks (viRead returns exact bytes, including NULs/newlines) until
    // the full IEEE 488.2 block has arrived, the cap is hit, or a read ends the
    // transfer (VI_SUCCESS) without more data pending.
    const ViUInt32 chunkSize = 65536;
    std::vector<unsigned char> chunk(chunkSize);
    size_t needed = 0;

    while (true) {
        if (cancelRequested_) {
            setLastError("Request cancelled by user");
            break;
        }

        const auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                                 std::chrono::steady_clock::now() - start).count();
        const long remaining = totalMs - elapsed;
        if (remaining <= 0) {
            setLastError("Timeout waiting for response");
            break;
        }
        if (viSetAttribute_) {
            viSetAttribute_(instrSession_, VI_ATTR_TMO_VALUE,
                            static_cast<ViUInt32>(std::min(sliceMs, remaining)));
        }

        ViUInt32 readCount = 0;
        ViStatus status = viRead_(instrSession_, chunk.data(), chunkSize, &readCount);

        if (readCount > 0) {
            buffer.insert(buffer.end(), chunk.data(), chunk.data() + readCount);
        }

        // A slice timeout with no new data just means "keep waiting".
        if (status == VI_ERROR_TMO && readCount == 0) {
            if (!buffer.empty()) {
                // We have a partial block but the instrument stalled; treat the
                // overall timeout (checked at loop top) as the deadline.
            }
            continue;
        }

        if (needed == 0) {
            ieee4882::BlockHeader header;
            std::string error;
            if (!ieee4882::parseHeader(buffer.data(), buffer.size(), header, error)) {
                break; // not a binary block
            }
            if (header.headerComplete && !header.indefinite) {
                needed = header.headerLength + header.payloadLength;
                if (needed > maxBytes) {
                    setLastError("Binary response exceeds maximum size ("
                                 + std::to_string(maxBytes) + " bytes)");
                    if (viSetAttribute_) {
                        viSetAttribute_(instrSession_, VI_ATTR_TMO_VALUE, oldTimeout);
                    }
                    return buffer;
                }
            } else if (header.headerComplete && header.indefinite) {
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

        // Stop when the instrument signalled end-of-transfer and gave no data,
        // or on a read error. VI_SUCCESS_MAX_CNT means more data is pending.
        if (status != VI_SUCCESS_MAX_CNT) {
            if (readCount == 0 && status != VI_SUCCESS) {
                if (buffer.empty()) {
                    setLastErrorFromStatus(status);
                }
                break;
            }
            if (status == VI_SUCCESS && needed == 0) {
                // Transfer ended and we never saw a definite-length header.
                break;
            }
        }
    }

    if (viSetAttribute_) {
        viSetAttribute_(instrSession_, VI_ATTR_TMO_VALUE, oldTimeout);
    }

    return buffer;
}

bool VISAConnection::isConnected() const
{
    return connected_;
}

std::string VISAConnection::getLastError() const
{
    return lastError_;
}

std::string VISAConnection::getConnectionType() const
{
    return "VISA";
}

std::string VISAConnection::getConnectionInfo() const
{
    if (connected_) {
        std::string interfaceType = VISAUtils::getInterfaceType(resourceString_);
        return "VISA (" + interfaceType + "): " + resourceString_;
    }
    return "VISA: Not connected";
}

bool VISAConnection::isAvailable() const
{
    // Try to load VISA library to check availability
    if (visaAvailable_) {
        return true;
    }

    // Try loading temporarily to check availability
    Platform::DynamicLibrary testLib;
#ifdef PLATFORM_WINDOWS
    std::string libName = "visa64";
#else
    std::string libName = "visa";
#endif
    
    if (testLib.load(libName)) {
        testLib.unload();
        return true;
    }
    
    return false;
}

void VISAConnection::setTimeout(unsigned int timeoutMs)
{
    timeoutMs_ = timeoutMs;
    
    if (connected_ && instrSession_ && viSetAttribute_) {
        viSetAttribute_(instrSession_, VI_ATTR_TMO_VALUE, timeoutMs_);
    }
}

bool VISAConnection::clear()
{
    if (!connected_ || !instrSession_) {
        setLastError("Not connected");
        return false;
    }

    if (!viClear_) {
        setLastError("VISA clear function not available");
        return false;
    }

    ViStatus status = viClear_(instrSession_);
    if (status != VI_SUCCESS) {
        setLastErrorFromStatus(status);
        return false;
    }

    return true;
}

void VISAConnection::setLastError(const std::string& error)
{
    lastError_ = error;
}

void VISAConnection::setLastErrorFromStatus(ViStatus status)
{
    if (viStatusDesc_ && defaultRM_) {
        char errStr[256];
        viStatusDesc_(defaultRM_, status, errStr);
        lastError_ = std::string(errStr);
    } else {
        lastError_ = "VISA error code: " + std::to_string(status);
    }
}

// Utility functions implementation
namespace VISAUtils {

std::string getInterfaceType(const std::string& resourceString)
{
    // Parse the interface type from resource string
    size_t pos = resourceString.find("::");
    if (pos != std::string::npos) {
        std::string interfaceType = resourceString.substr(0, pos);
        // Convert to uppercase
        for (size_t i = 0; i < interfaceType.length(); ++i) {
            interfaceType[i] = static_cast<char>(std::toupper(static_cast<unsigned char>(interfaceType[i])));
        }
        return interfaceType;
    }
    return "";
}

bool isValidResourceString(const std::string& resourceString)
{
    if (resourceString.empty()) {
        return false;
    }

    // Basic validation - should contain "::"
    if (resourceString.find("::") == std::string::npos) {
        return false;
    }

    std::string interfaceType = getInterfaceType(resourceString);
    if (interfaceType.empty()) {
        return false;
    }
    
    // Check for known interface types (allowing variations with numbers)
    return (interfaceType.find("TCPIP") == 0 || 
            interfaceType.find("GPIB") == 0 || 
            interfaceType.find("USB") == 0 || 
            interfaceType.find("ASRL") == 0 ||
            interfaceType.find("PXI") == 0 ||
            interfaceType.find("VXI") == 0);
}

std::string buildTCPIPResource(const std::string& ip, int port)
{
    std::ostringstream oss;
    oss << "TCPIP::" << ip;
    
    if (port > 0) {
        oss << "::" << port << "::SOCKET";
    } else {
        oss << "::INSTR";
    }
    
    return oss.str();
}

std::string buildGPIBResource(int board, int address)
{
    std::ostringstream oss;
    oss << "GPIB" << board << "::" << address << "::INSTR";
    return oss.str();
}

} // namespace VISAUtils
