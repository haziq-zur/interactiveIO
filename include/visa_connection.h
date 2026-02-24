#ifndef VISA_CONNECTION_H
#define VISA_CONNECTION_H

#include "instrument_connection.h"
#include <string>

// VISA types - these will be loaded dynamically at runtime
typedef unsigned long ViStatus;
typedef unsigned long ViSession;
typedef unsigned long ViUInt32;
typedef unsigned char* ViBuf;
typedef char* ViRsrc;
typedef ViUInt32 ViAttr;

// Forward declare function pointers
typedef ViStatus (*ViOpenDefaultRMFunc)(ViSession*);
typedef ViStatus (*ViOpenFunc)(ViSession, ViRsrc, ViUInt32, ViUInt32, ViSession*);
typedef ViStatus (*ViCloseFunc)(ViSession);
typedef ViStatus (*ViWriteFunc)(ViSession, ViBuf, ViUInt32, ViUInt32*);
typedef ViStatus (*ViReadFunc)(ViSession, ViBuf, ViUInt32, ViUInt32*);
typedef ViStatus (*ViSetAttributeFunc)(ViSession, ViAttr, ViUInt32);
typedef ViStatus (*ViClearFunc)(ViSession);
typedef ViStatus (*ViStatusDescFunc)(ViSession, ViStatus, char*);

#define VI_SUCCESS 0L
#define VI_NULL 0
#define VI_ATTR_TMO_VALUE 0x3FFF001A

/**
 * @brief VISA (Virtual Instrument Software Architecture) connection implementation
 * 
 * Supports multiple instrument interfaces: GPIB, USB, VXI-11, Serial
 * Uses dynamic loading to work even when VISA is not installed.
 */
class VISAConnection : public IInstrumentConnection {
public:
    VISAConnection();
    virtual ~VISAConnection() override;

    // IInstrumentConnection interface implementation
    virtual bool initialize() override;
    virtual bool connect(const std::string& address, int port = 0) override;
    virtual void disconnect() override;
    virtual bool sendCommand(const std::string& command) override;
    virtual std::string readResponse(int timeoutSeconds = 10) override;
    virtual bool isConnected() const override;
    virtual std::string getLastError() const override;
    virtual std::string getConnectionType() const override;
    virtual std::string getConnectionInfo() const override;
    virtual bool isAvailable() const override;

    // VISA specific methods
    std::string getResourceString() const { return resourceString_; }
    void setTimeout(unsigned int timeoutMs);
    unsigned int getTimeout() const { return timeoutMs_; }
    bool clear();

private:
    std::string resourceString_;
    std::string lastError_;
    bool connected_;
    bool rmInitialized_;
    bool visaAvailable_;
    unsigned int timeoutMs_;
    void* visaDll_;  // HMODULE handle
    
    ViSession defaultRM_;
    ViSession instrSession_;
    
    // Function pointers
    ViOpenDefaultRMFunc viOpenDefaultRM_;
    ViOpenFunc viOpen_;
    ViCloseFunc viClose_;
    ViWriteFunc viWrite_;
    ViReadFunc viRead_;
    ViSetAttributeFunc viSetAttribute_;
    ViClearFunc viClear_;
    ViStatusDescFunc viStatusDesc_;
    
    void setLastError(const std::string& error);
    void setLastErrorFromStatus(ViStatus status);
    bool loadVISALibrary();
    void unloadVISALibrary();
};

// Utility functions for VISA
namespace VISAUtils {
    // Parse VISA resource string to get interface type
    std::string getInterfaceType(const std::string& resourceString);
    
    // Validate VISA resource string format
    bool isValidResourceString(const std::string& resourceString);
    
    // Build TCPIP resource string from IP and port
    std::string buildTCPIPResource(const std::string& ip, int port = -1);
    
    // Build GPIB resource string
    std::string buildGPIBResource(int board, int address);
}

#endif // VISA_CONNECTION_H
