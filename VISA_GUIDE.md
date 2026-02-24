# VISA Configuration Guide

This guide explains how to configure and use VISA (Virtual Instrument Software Architecture) support in interactiveIO.

## Prerequisites

### Installing NI-VISA

1. Download NI-VISA from [National Instruments](https://www.ni.com/en-us/support/downloads/drivers/download.ni-visa.html)
2. Run the installer
3. Choose the appropriate version:
   - **64-bit** recommended for x64 systems
   - **32-bit** for x86 systems
4. Follow the installation wizard
5. Restart your computer if prompted

### Verifying VISA Installation

After installation, verify that VISA is installed:

**Windows:**
- Check for: `C:\Program Files\IVI Foundation\VISA\`
- NI MAX (Measurement & Automation Explorer) should be available

## Building with VISA Support

### CMake Configuration

```bash
# Basic VISA build
cmake -B build -S . -DUSE_VISA=ON

# Build with specific configuration
cmake -B build -S . -DUSE_VISA=ON -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```

### Manual Library Paths

If CMake cannot find VISA automatically:

```bash
# 64-bit VISA
cmake -B build -S . ^
  -DUSE_VISA=ON ^
  -DVISA_LIBRARY="C:/Program Files/IVI Foundation/VISA/Win64/Lib_x64/msc/visa64.lib" ^
  -DVISA_INCLUDE_DIR="C:/Program Files/IVI Foundation/VISA/Win64/Include"

# 32-bit VISA
cmake -B build -S . ^
  -DUSE_VISA=ON ^
  -DVISA_LIBRARY="C:/Program Files/IVI Foundation/VISA/WinNT/Lib/msc/visa32.lib" ^
  -DVISA_INCLUDE_DIR="C:/Program Files/IVI Foundation/VISA/WinNT/Include"
```

## VISA Resource Strings

### Format

VISA resource strings follow this general format:
```
INTERFACE[board]::ADDRESS[::PROTOCOL]::INSTR
```

### Examples by Interface Type

#### TCP/IP (Ethernet)

**VXI-11 Protocol (Recommended for LAN instruments):**
```
TCPIP::192.168.1.100::INSTR
TCPIP::instrument.local::INSTR
```

**Raw Socket:**
```
TCPIP::192.168.1.100::5025::SOCKET
```

#### GPIB (IEEE 488)

```
GPIB0::1::INSTR           # Board 0, Address 1
GPIB0::10::INSTR          # Board 0, Address 10
GPIB1::5::INSTR           # Board 1, Address 5
```

#### USB

```
USB0::0x1234::0x5678::SN123456::INSTR
```
Where:
- `0x1234` = Vendor ID
- `0x5678` = Product ID
- `SN123456` = Serial Number

#### Serial (RS-232)

```
ASRL1::INSTR              # COM1
ASRL2::INSTR              # COM2
```

## Using the VISA Application

### Interactive Mode

```bash
cd build\bin\Release
interactiveIO_VISA.exe
```

The application will prompt for a resource string. You can enter:

**Full resource strings:**
```
TCPIP::192.168.1.100::INSTR
```

**Helper commands:**
```
tcpip 192.168.1.100                    # Creates TCPIP::192.168.1.100::INSTR
socket 192.168.1.100 5025              # Creates TCPIP::192.168.1.100::5025::SOCKET
gpib 0 1                               # Creates GPIB0::1::INSTR
```

### Common Commands

Once connected:

```
> *IDN?                   # Query instrument identification
Response: Manufacturer,Model,Serial,Version

> *RST                    # Reset instrument

> timeout 5000            # Set 5 second timeout

> clear                   # Clear instrument buffer

> help                    # Show available commands

> exit                    # Disconnect and exit
```

## Interface Comparison

### When to Use Each Interface

| Interface | Use Case | Advantages | Disadvantages |
|-----------|----------|------------|---------------|
| **TCP/IP Socket** | Direct network access | Fast, simple | No instrument discovery |
| **VXI-11 (VISA TCPIP)** | LAN instruments | Standard, robust | Requires VISA |
| **GPIB** | Legacy instruments | High reliability | Requires GPIB hardware |
| **USB** | Modern instruments | Plug-and-play | Driver dependent |
| **Serial** | Simple instruments | Universal | Slow, manual config |

### Performance

- **TCP/IP Raw Socket**: Fastest, lowest latency
- **VXI-11**: Standard performance, good for most applications
- **GPIB**: Moderate speed, very reliable
- **USB**: Fast, but variable by implementation
- **Serial**: Slow (9600-115200 baud typical)

## Troubleshooting

### VISA Not Found

**Error:** `VISA library not found`

**Solutions:**
1. Verify NI-VISA is installed
2. Check installation path matches CMake hints
3. Manually specify library paths (see above)
4. Ensure 32-bit/64-bit match between compiler and VISA

### Connection Failures

**Error:** `Failed to connect: VI_ERROR_RSRC_NFOUND`

**Solutions:**
1. Verify instrument is powered on
2. Check network/cable connections
3. Ping the instrument (for TCP/IP)
4. Use NI MAX to scan for instruments
5. Verify resource string format

**Error:** `Timeout waiting for response`

**Solutions:**
1. Increase timeout: `timeout 30000` (30 seconds)
2. Check if instrument is busy
3. Verify command syntax
4. For queries, ensure command ends with `?`

### Build Issues

**Error:** `Cannot open include file: 'visa.h'`

**Solution:** VISA include path not found:
```bash
cmake -B build -S . -DUSE_VISA=ON -DVISA_INCLUDE_DIR="<path>"
```

**Error:** `LNK1104: cannot open file 'visa64.lib'`

**Solution:** VISA library path not found:
```bash
cmake -B build -S . -DUSE_VISA=ON -DVISA_LIBRARY="<path>"
```

## Testing VISA Support

### Run VISA Tests

```bash
cd build
ctest -C Debug -R VISA --output-on-failure
```

This runs only VISA-related tests.

### Verify Without Instruments

All VISA utility functions work without hardware:
- Resource string validation
- Resource string building
- Interface type detection

### Test with Real Instrument

1. Connect instrument to PC
2. Use NI MAX to find resource string
3. Run `interactiveIO_VISA.exe`
4. Enter resource string
5. Send `*IDN?` to verify communication

## Integration in Your Code

### Using VISAConnection Class

```cpp
#include "visa_connection.h"

int main() {
    // Check VISA availability
    if (!VISAUtils::isVISAAvailable()) {
        std::cerr << "VISA not available\n";
        return 1;
    }
    
    // Create connection
    VISAConnection visa;
    
    // Initialize and connect
    if (!visa.initialize() || 
        !visa.connect("TCPIP::192.168.1.100::INSTR")) {
        std::cerr << "Error: " << visa.getLastError() << "\n";
        return 1;
    }
    
    // Query instrument
    std::string id = visa.query("*IDN?");
    std::cout << "Instrument: " << id << "\n";
    
    // Send command
    visa.sendCommand("*RST");
    
    // Cleanup automatic via destructor
    return 0;
}
```

### Building Resource Strings Programmatically

```cpp
#include "visa_connection.h"

// Build TCPIP resource
std::string resource = VISAUtils::buildTCPIPResource("192.168.1.100");
// Result: "TCPIP::192.168.1.100::INSTR"

// Build socket resource
std::string socket = VISAUtils::buildTCPIPResource("192.168.1.100", 5025);
// Result: "TCPIP::192.168.1.100::5025::SOCKET"

// Build GPIB resource
std::string gpib = VISAUtils::buildGPIBResource(0, 1);
// Result: "GPIB0::1::INSTR"

// Validate before use
if (VISAUtils::isValidResourceString(resource)) {
    visa.connect(resource);
}
```

## Additional Resources

- [NI-VISA Documentation](https://www.ni.com/docs/en-US/bundle/ni-visa/)
- [VISA Specification](https://www.ivifoundation.org/specifications/)
- [VXI-11 Protocol](https://www.vxibus.org/specifications.html)
- [SCPI Commands](https://www.ivifoundation.org/scpi/)
