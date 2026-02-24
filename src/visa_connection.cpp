#include "visa_connection.h"
#include "platform/dynamic_loader.h"
#include <iostream>
#include <sstream>
#include <string>
#include <algorithm>
#include <cctype>

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

    // Temporarily set timeout
    unsigned int oldTimeout = timeoutMs_;
    if (viSetAttribute_) {
        viSetAttribute_(instrSession_, VI_ATTR_TMO_VALUE, timeoutSeconds * 1000);
    }

    const unsigned int bufferSize = 1024;
    char* buffer = new char[bufferSize];
    ViUInt32 readCount = 0;
    
    ViStatus status = viRead_(instrSession_, 
                              (unsigned char*)buffer, 
                              bufferSize - 1, 
                              &readCount);
    
    std::string result;
    if (status == VI_SUCCESS || readCount > 0) {
        buffer[readCount] = '\0';
        result = std::string(buffer);
    } else {
        setLastErrorFromStatus(status);
    }

    delete[] buffer;

    // Restore timeout
    if (viSetAttribute_) {
        viSetAttribute_(instrSession_, VI_ATTR_TMO_VALUE, oldTimeout);
    }
    
    return result;
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
