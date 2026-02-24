# interactiveIO

Interactive SCPI instrument communication tool with unified interface supporting both TCP/IP (Winsock2) and VISA protocols.

## Features

- **Unified Application**: Single executable with runtime protocol selection
- **TCP/IP Communication**: Direct socket communication using Winsock2
- **VISA Protocol Support**: Industry-standard VISA for GPIB, USB, VXI-11, and Serial instruments
- **Dynamic VISA Loading**: Works even without NI-VISA installed (loads DLL at runtime)
- **Interactive Menu**: User-friendly protocol selection and configuration
- **Protocol-Specific Features**: Timeout control, buffer clearing, connection management
- **Comprehensive Testing**: Full unit test coverage with Google Test
- **Debug Support**: Debug and Release build configurations

## Quick Start

### Run the Application

```bash
.\build\bin\Release\interactiveIO.exe
```

The application will display a menu to select your communication protocol:

```
=================================================
  Interactive Instrument Communication Tool
  Supports: TCP/IP Socket & VISA Protocols
=================================================

Select Communication Protocol:
  1. TCP/IP Socket (Direct Winsock2)
  2. VISA (GPIB, USB, VXI-11, Serial)
  3. Exit
```

### TCP/IP Example

1. Select option `1` for TCP/IP Socket
2. Choose whether to connect now (y/n)
3. If connecting now:
   - Enter IP address: `192.168.1.100`
   - Enter port: `5025`
4. Send SCPI commands:
   ```
   > *IDN?
   Response: Manufacturer,Model,Serial,Version
   
   > *RST
   Command sent.
   
   > disconnect
   Disconnected from instrument
   
   > connect
   Enter IP address: 192.168.1.101
   Enter port: 5025
   Connected successfully!
   ```

### VISA Example

1. Select option `2` for VISA
2. Enter resource string (or use helper commands):
   - Direct: `TCPIP::192.168.1.100::INSTR`
   - Helper: `tcpip 192.168.1.100`
   - GPIB: `gpib 0 1`
3. Send SCPI commands:
   ```
   > *IDN?
   Response: Manufacturer,Model,Serial,Version
   
   > timeout
   Enter timeout in milliseconds: 5000
   Timeout set to 5000 ms
   
   > clear
   Instrument buffer cleared
   ```

### Available Commands in Session

- `help` - Show available commands
- `info` - Display connection information and current EOL sequence
- `connect` - Connect/reconnect to instrument
- `disconnect` - Disconnect from current instrument
- `eol` - Set EOL (End of Line) sequence (default: `\n`)
- `timeout` - Set timeout (VISA only)
- `clear` - Clear instrument buffer (VISA only)
- `exit` - Return to protocol menu
- `quit` - Exit program
- Any SCPI command your instrument supports

### Advanced Features

**EOL Sequence Configuration**

By default, commands are terminated with `\n` (newline). You can customize this for instruments with different requirements:

```
> eol
Current EOL sequence: \n
Enter new EOL sequence (use \n, \r, \t for special chars): \r\n
EOL sequence set to: \r\n

> info
Connected to: 192.168.1.100:5025
Type: TCP/IP Socket
EOL Sequence: \r\n
```

Supported escape sequences:
- `\n` - Newline (LF)
- `\r` - Carriage return (CR)
- `\t` - Tab
- `\\` - Backslash

**Connect/Disconnect**

You can disconnect and reconnect without leaving the session:

```
> disconnect
Disconnected from instrument

> connect
--- TCP/IP Socket Configuration ---
Enter IP address (e.g., 192.168.1.100): 192.168.1.200
Enter port (e.g., 5025): 5025
Connecting to 192.168.1.200:5025...
Connected successfully!
```

## Architecture

The project uses a clean interface-based architecture:

```
IInstrumentConnection (Base Interface)
    |
    +-- TCPIPConnection (Winsock2 implementation)
    |
    +-- VISAConnection (Dynamic VISA loading)
```

Both protocols are always compiled. VISA support uses dynamic DLL loading, so the application works even without NI-VISA installed.

See [REFACTORING_SUMMARY.md](REFACTORING_SUMMARY.md) for detailed architecture documentation.

## Building with CMake

### Prerequisites
- CMake 3.15 or higher
- A C++11 compatible compiler (MSVC, MinGW-w64, or Clang)
- Windows SDK (for Winsock2)
- **Optional**: NI-VISA or compatible VISA library (for VISA protocol support)

### Build Instructions

#### Quick Build with Batch Scripts (Windows):

**Release Build:**
```bash
build.bat
```

**Debug Build with Tests:**
```bash
build_debug.bat
```

**Custom Build:**
```bash
build.bat [debug|release] [test]
```

Examples:
- `build.bat` - Release build
- `build.bat debug` - Debug build
- `build.bat release test` - Release build + run tests
- `build.bat debug test` - Debug build + run tests

#### Using CMake GUI or Command Line:

1. **Create a build directory:**
   ```bash
   mkdir build
   cd build
   ```

2. **Configure the project:**
   ```bash
   cmake ..
   ```
   
   Or specify a generator explicitly:
   ```bash
   cmake -G "Visual Studio 17 2022" ..
   # or
   cmake -G "MinGW Makefiles" ..
   # or
   cmake -G "Ninja" ..
   ```

3. **Build the project:**
   ```bash
   # Release build
   cmake --build . --config Release
   
   # Debug build
   cmake --build . --config Debug
   ```

4. **Run the executable:**
   ```bash
   # Release
   .\bin\Release\interactiveIO.exe
   
   # Debug
   .\bin\Debug\interactiveIO.exe
   ```

#### Alternative: One-line build command
```bash
# Release build
cmake -B build -S . && cmake --build build --config Release

# Debug build
cmake -B build -S . -DCMAKE_BUILD_TYPE=Debug && cmake --build build --config Debug
```

## VISA Protocol Support

The project supports the VISA (Virtual Instrument Software Architecture) protocol for communication with instruments over various interfaces including GPIB, USB, Ethernet (VXI-11), and serial.

**📖 For detailed VISA configuration instructions, see [VISA_GUIDE.md](VISA_GUIDE.md)**

### Enabling VISA Support

To build with VISA support, you need to:

1. **NI-VISA is Optional**
   - The application works without NI-VISA installed
   - VISA DLL is loaded dynamically at runtime
   - If you want VISA support, download from [National Instruments](https://www.ni.com/en-us/support/downloads/drivers/download.ni-visa.html)

2. **Configure and Build:**
   ```bash
   cmake -B build -S .
   cmake --build build --config Release
   ```

Both protocols are always compiled. No special configuration needed!

### VISA Resource Strings

The unified application supports various VISA resource strings:

- **TCP/IP (VXI-11)**: `TCPIP::192.168.1.1::INSTR` or helper: `tcpip 192.168.1.1`
- **TCP/IP (Socket)**: `TCPIP::192.168.1.1::5025::SOCKET` or helper: `socket 192.168.1.1 5025`
- **GPIB**: `GPIB0::1::INSTR` or helper: `gpib 0 1`
- **USB**: `USB0::0x1234::0x5678::SN123::INSTR`
- **Serial**: `ASRL1::INSTR`

### VISA vs TCP/IP

| Feature | TCP/IP (Winsock2) | VISA |
|---------|-------------------|------|
| GPIB Support | ❌ No | ✅ Yes |
| USB Support | ❌ No | ✅ Yes |
| VXI-11 Support | ❌ No | ✅ Yes |
| Raw Socket | ✅ Yes | ✅ Yes |
| Dependencies | Windows only | NI-VISA required |
| Performance | Fast | Standard |

## Debug Configuration

The project supports debug builds with the following features:

- **MSVC Debug Flags**: `/MDd /Zi /Ob0 /Od /RTC1`
- **GCC/Clang Debug Flags**: `-g -O0 -DDEBUG`
- **Debug Macro**: `_DEBUG` is defined in debug builds

### Building in Debug Mode

Debug builds include debug symbols and runtime checks, useful for:
- Debugging with Visual Studio or other debuggers
- Runtime error detection
- Memory leak detection tools
- Performance profiling

## Unit Testing

The project includes comprehensive unit tests using Google Test framework. Tests are automatically downloaded during CMake configuration.

### Running Tests

**Using batch script:**
```bash
build.bat debug test
```

**Using CMake/CTest:**
```bash
cd build
ctest -C Debug --output-on-failure
```

**Running tests directly:**
```bash
.\build\bin\Debug\scpi_tests.exe
```

### Test Coverage

The test suite includes:
- **Utility Functions Tests** (`test_scpi_utils.cpp`)
  - IP address validation
  - Command formatting (newline handling)
  - Query command detection
  - Edge cases and performance tests

- **TCP/IP Connection Tests** (`test_scpi_connection.cpp`)
  - Connection initialization
  - Error handling
  - State management
  - Invalid input handling
  - RAII behavior

- **VISA Connection Tests** (`test_visa_connection.cpp`)
  - VISA availability detection
  - Resource string validation
  - Resource string building utilities
  - Connection state management
  - Error handling with and without VISA compiled

### Adding New Tests

To add new tests:
1. Create a new test file in the `tests/` directory
2. Add it to `tests/CMakeLists.txt`
3. Follow the Google Test framework conventions

### Installation (Optional)
```bash
cmake --install build --prefix <installation_path>
```

## Project Structure

```
interactiveIO/
├── interactiveIO/
│   ├── interactiveIO.cpp         # Main TCP/IP application
│   ├── interactiveIO_visa.cpp    # VISA application (optional)
│   ├── scpi_core.h               # TCP/IP connection class header
│   ├── scpi_core.cpp             # TCP/IP connection implementation
│   ├── visa_connection.h         # VISA connection class header
│   └── visa_connection.cpp       # VISA connection implementation
├── tests/
│   ├── CMakeLists.txt            # Test build configuration
│   ├── test_scpi_utils.cpp       # Utility functions tests
│   ├── test_scpi_connection.cpp  # TCP/IP connection tests
│   └── test_visa_connection.cpp  # VISA connection tests
├── build/                        # Build output directory (generated)
├── CMakeLists.txt                # Main CMake configuration
├── build.bat                     # Windows build script
├── build_debug.bat               # Quick debug build script
└── README.md                     # This file
```

### Core Components

**SCPIConnection Class** (`scpi_core.h/cpp`)
- Manages TCP/IP socket connections to SCPI instruments
- Handles Winsock initialization and cleanup
- Provides methods for sending commands and receiving responses
- Implements timeout handling and error reporting

**VISAConnection Class** (`visa_connection.h/cpp`)
- Manages VISA connections to instruments
- Supports multiple interface types (GPIB, USB, VXI-11, Serial)
- Provides standard VISA operations (read, write, query, clear)
- Gracefully handles missing VISA libraries

**SCPIUtils Namespace** (`scpi_core.h/cpp`)
- `isValidIPAddress()` - Validates IP address format
- `ensureNewline()` - Ensures commands end with newline
- `isQueryCommand()` - Detects query commands (commands ending with '?')

**VISAUtils Namespace** (`visa_connection.h/cpp`)
- `isVISAAvailable()` - Checks if VISA support is compiled in
- `isValidResourceString()` - Validates VISA resource string format
- `buildTCPIPResource()` - Builds TCPIP resource strings
- `buildGPIBResource()` - Builds GPIB resource strings
- `getInterfaceType()` - Extracts interface type from resource string

**Main Applications**
- `interactiveIO.cpp` - TCP/IP socket-based communication
- `interactiveIO_visa.cpp` - VISA-based communication (when enabled)

## Usage

### TCP/IP Communication (interactiveIO.exe)

The program connects to SCPI instruments via TCP/IP sockets. By default, it's configured to connect to:
- IP: 141.183.190.1
- Port: 5025

Modify these values in the source code as needed for your instrument.

Once connected, type SCPI commands interactively. Type `exit` to quit.

**Example:**
```bash
cd build\bin\Release
interactiveIO.exe
> *IDN?
Response: Manufacturer,Model,Serial,Version
> *RST
> exit
```

### VISA Communication (interactiveIO_VISA.exe)

When built with VISA support, use the VISA application for more flexibility:

```bash
cd build\bin\Release
interactiveIO_VISA.exe
```

The program will prompt for a VISA resource string. You can use:
- Full resource strings: `TCPIP::192.168.1.1::INSTR`
- Helper commands:
  - `tcpip <ip>` - Creates TCPIP::IP::INSTR
  - `socket <ip> <port>` - Creates TCPIP::IP::PORT::SOCKET
  - `gpib <board> <address>` - Creates GPIB resource string

**Interactive Commands:**
- `help` - Show available commands
- `timeout <ms>` - Set timeout in milliseconds
- `clear` - Clear instrument buffer
- `exit` - Exit the program

**Example Session:**
```
Enter VISA resource string: tcpip 192.168.1.1
Using resource: TCPIP::192.168.1.1::INSTR
Connected successfully!
> *IDN?
Response: Manufacturer,Model,Serial,Version
> timeout 5000
Timeout set to 5000 ms
> clear
Instrument buffer cleared
> exit
```