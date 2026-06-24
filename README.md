# interactiveIO

Interactive SCPI instrument communication tool with unified interface supporting both TCP/IP and VISA protocols.

## Features

- **Dual Interface**: Qt6 GUI and Console applications available
- **Command-Line API**: One-shot CLI that performs an instrument operation and prints a JSON result (scripts, CI, automation)
- **Screen Image Capture**: Retrieve instrument screenshots (PNG/BMP/JPEG/GIF) via IEEE 488.2 binary block transfer, available in the GUI, console, and CLI API
- **Cross-Platform Support**: Single codebase for Windows and Linux
- **Unified Application**: Single executable with runtime protocol selection
- **TCP/IP Communication**: Cross-platform socket communication (Winsock2 on Windows, Berkeley sockets on Linux)
- **VISA Protocol Support**: Industry-standard VISA for GPIB, **USB**, VXI-11, and Serial instruments
- **USB Instrument Support**: Quick USB helper for easy Vendor/Product ID configuration
- **Dynamic VISA Loading**: Works even without NI-VISA installed (loads library at runtime)
- **Modern Dark Mode GUI**: Qt6 interface with color-coded output, tooltips, and connection helpers
- **Interactive Console**: Command-line interface for automation and scripting
- **Protocol-Specific Features**: Timeout control, buffer clearing, connection management
- **Command History**: Navigate previous commands with Up/Down arrows (GUI)
- **Comprehensive Testing**: Full unit test coverage with Google Test and Qt Test
- **Debug Support**: Debug and Release build configurations

## Platform Support

### Windows ✅
- MinGW-w64 (GCC 13.x, bundled with Qt) or MSVC 2022
- Qt 6.10+ (Core, Widgets, Test, Svg) for the GUI
- CMake 3.15+ and Ninja
- Uses Winsock2 for networking
- Loads `visa64.dll` or `visa32.dll` dynamically

### Linux ✅ (Ready for Testing)
- GCC 11+ or Clang 14+
- CMake 3.15+
- Uses Berkeley sockets (built-in)
- Loads `libvisa.so` dynamically

### macOS (Future)
- Platform abstraction layer ready
- Requires testing on macOS hardware

## Quick Start

### Windows

#### GUI Application (Recommended)

```powershell
.\build\bin\Release\interactiveIO-gui.exe
```

#### Console Application

```powershell
.\build\bin\Release\interactiveIO.exe
```

#### Command-Line API

```powershell
.\build\bin\Release\interactiveIO-api.exe --address 192.168.1.100 --command "*IDN?"
```

Each invocation connects, runs one action and prints a JSON result (see [Command-Line API](#command-line-api)).

**Or create a distribution package:**

```powershell
.\scripts\package.ps1
```

This bundles the GUI together with every runtime dependency it needs (Qt DLLs +
plugins via `windeployqt`, plus the MinGW runtime DLLs) and the standalone
console/API executables, then compresses everything into
`dist\interactiveIO-v<version>-Windows-x64.zip`. The packaged application:
- ✅ Runs on any Windows 10+ machine with no Qt or compiler installed
- ✅ No runtime dependencies for TCP/IP connections
- ✅ No installation required - just extract and run
- ⚠️ VISA support requires NI-VISA installation (optional)

### Linux

```bash
# Build
cmake -B build
cmake --build build

# Run
./build/bin/interactiveIO

# Run tests
cd build && ctest
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
- `image` - Capture a screen image from the instrument and save it to a file
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

## Command-Line API

The command-line API exposes the same instrument controller used by the GUI and
console as a one-shot CLI: each invocation opens a connection, performs a single
action, prints a JSON object to stdout, and exits. This makes it easy to drive
from scripts, CI, or any language that can run a process and parse JSON. It is
built by default (disable with `-DBUILD_API=OFF`).

### Usage

```powershell
# Windows
.\build\bin\Release\interactiveIO-api.exe --address 192.168.1.100 --command "*IDN?"
```

```bash
# Linux
./build/bin/interactiveIO-api --address 192.168.1.100 --command "*IDN?"
```

**Connection options**

| Option | Description | Default |
| --- | --- | --- |
| `--protocol <tcpip\|visa>` | Transport to use | `tcpip` |
| `--address <addr>` | Host IP (tcpip) or full VISA resource string | — |
| `--port <n>` | TCP port (tcpip only) | `5025` |

For VISA, set `--address` to the full resource string, e.g.
`TCPIP::192.168.1.100::INSTR`.

**Settings (applied before the action)**

| Option | Description |
| --- | --- |
| `--eol <lf\|cr\|crlf\|none>` | End-of-line appended to commands (default `lf`) |
| `--response-timeout <n>` | Response timeout in seconds |
| `--no-response-time` | Do not measure elapsed time |
| `--set-timeout-ms <n>` | VISA I/O timeout in milliseconds |

**Actions (choose one)**

| Option | Description |
| --- | --- |
| `--command "<scpi>"` | Send a SCPI command / query (`--timeout-seconds <n>` overrides the response timeout for this call) |
| `--clear` | VISA device clear |
| `--image "<scpi>"` | Capture an image (`--format`, `--output <file>`, `--max-bytes <n>`) |
| `--status` | Report connection status only |
| `--health` | Print service health/version (no connection) |
| `--help` | Show usage and exit |

### Exit Codes

| Exit code | Meaning |
| --- | --- |
| `0` | Success |
| `1` | Operational failure (connection, send, timeout, etc.) |
| `2` | Usage / invalid input error |

### Response Format

Every instrument operation prints a JSON object built from the controller's
structured result:

```json
{
  "success": true,
  "message": "Response received",
  "response": "Keysight,34470A,MY12345678,A.02.17",
  "elapsedMs": 14.382,
  "code": "None",
  "severity": "INFO"
}
```

Failures carry a machine-readable `code`, a `severity`, and a ready-to-display
`error` string, and the process exits with a non-zero code:

```json
{
  "success": false,
  "message": "Not connected",
  "elapsedMs": 0.0,
  "code": "NotConnected",
  "severity": "ERROR",
  "error": "[ERROR] NotConnected: Not connected"
}
```

### Examples

```bash
# Service health / version (no connection)
interactiveIO-api --health

# Query the instrument identity over TCP/IP
interactiveIO-api --address 192.168.1.100 --port 5025 --command "*IDN?"

# Query over VISA (VXI-11)
interactiveIO-api --protocol visa --address "TCPIP::192.168.1.100::INSTR" --command "*IDN?"

# Send a write command with a custom EOL
interactiveIO-api --address 192.168.1.100 --eol crlf --command "*RST"

# VISA device clear
interactiveIO-api --protocol visa --address "TCPIP::192.168.1.100::INSTR" --clear

# Connection status only
interactiveIO-api --address 192.168.1.100 --status
```

```powershell
# PowerShell: parse the JSON result
$r = .\build\bin\Release\interactiveIO-api.exe --address 192.168.1.100 --command "*IDN?" | ConvertFrom-Json
$r.response
```


## Image Capture

Many instruments can return a screenshot of their display as an IEEE 488.2
arbitrary binary block (e.g. `#800012345<...PNG bytes...>`). The application
parses the block header, validates the image signature, and exposes the raw
bytes through all three frontends. PNG, BMP, JPEG, and GIF are auto-detected.

The exact SCPI command varies by vendor, for example:

| Vendor (example) | Command |
| --- | --- |
| Keysight / Agilent | `:DISPlay:DATA? PNG` |
| Rohde & Schwarz | `HCOPy:DATA?` |
| Tektronix | `HARDCopy:DATA?` |

> Consult your instrument's programming manual for the correct command and any
> required setup (e.g. `:HCOPy:FORMat PNG`).

### GUI

Click **Capture Screen** in the toolbar. The screenshot runs on a background
thread with a loading overlay, then opens a preview dialog with a **Save…**
button. Configure the capture command and expected format under
**Settings → Capture command / Capture format**.

### Console

```
> image
Enter SCPI capture command (e.g. :DISPlay:DATA? PNG): :DISPlay:DATA? PNG
Expected format (png/bmp/jpg/gif, blank to auto-detect):
Enter output file path (default: capture.png): screen.png
Saved 18342 bytes (png) to screen.png
```

### Command-Line API

`--image "<scpi>"` captures an image. It accepts an optional `--format` hint, an
optional `--max-bytes` cap (default 32 MiB), and an optional `--output <file>`.
Without `--output` the raw bytes are base64-encoded into the JSON result; with
`--output` they are written straight to the file.

```bash
# Base64 JSON envelope (default)
interactiveIO-api --address 192.168.1.100 --image ":DISPlay:DATA? PNG" --format png

# Raw bytes written straight to a file
interactiveIO-api --address 192.168.1.100 --image ":DISPlay:DATA? PNG" --output screen.png
```

A base64 response includes the decoded payload alongside the standard result
fields:

```json
{
  "success": true,
  "message": "Captured 18342 bytes (png)",
  "elapsedMs": 87.4,
  "code": "None",
  "severity": "INFO",
  "format": "png",
  "bytes": 18342,
  "encoding": "base64",
  "dataBase64": "iVBORw0KGgoAAAANSUhEUgAA..."
}
```

With `--output` the result instead reports the file it wrote:

```json
{
  "success": true,
  "message": "Captured 18342 bytes (png)",
  "format": "png",
  "bytes": 18342,
  "encoding": "file",
  "output": "screen.png"
}
```

Oversized responses fail with code `ResponseTooLarge`, malformed blocks with
`MalformedBlock`, and unrecognised image data with `UnsupportedImageFormat`.

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


## Building with CMake

### Prerequisites
- CMake 3.15 or higher
- A C++17 compatible compiler (MSVC, MinGW-w64, or Clang)
- Windows SDK (for Winsock2)
- **Network access on first configure**: the command-line API fetches `nlohmann/json` via CMake `FetchContent` (same as Google Test). Disable the API with `-DBUILD_API=OFF` to build offline.
- **Optional**: Qt6 6.10+ with the Svg module (for GUI application) - Download from https://www.qt.io/download-qt-installer
- **Optional**: NI-VISA or compatible VISA library (for VISA protocol support)

### Build Instructions

#### Quick Build with Batch Scripts (Windows):

**Release Build:**
```bash
.\scripts\build.bat
```

**Debug Build with Tests:**
```bash
.\scripts\build_debug.bat
```

**Custom Build:**
```bash
.\scripts\build.bat [debug|release] [test]
```

Examples:
- `scripts\build.bat` - Release build
- `scripts\build.bat debug` - Debug build
- `scripts\build.bat release test` - Release build + run tests
- `scripts\build.bat debug test` - Debug build + run tests

#### Using CMake GUI or Command Line:

1. **Create a build directory:**
   ```bash
   mkdir build
   cd build
   ```

2. **Configure the project:**

   The recommended way is the bundled CMake preset (MinGW + Qt + Ninja):
   ```bash
   cmake --preset mingw-qt
   cmake --build --preset mingw-qt
   ```

   Or configure manually:
   ```bash
   cmake ..
   ```
   
   Or specify a generator explicitly:
   ```bash
   cmake -G "Ninja" ..
   # or
   cmake -G "MinGW Makefiles" ..
   # or
   cmake -G "Visual Studio 17 2022" ..
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
   # GUI Application (if Qt6 is available)
   .\bin\Release\interactiveIO-gui.exe
   
   # Console Application
   .\bin\Release\interactiveIO.exe
   
   # Debug builds
   .\bin\Debug\interactiveIO-gui.exe
   .\bin\Debug\interactiveIO.exe
   ```

5. **Building with Qt6 GUI:**
   
   If Qt6 is not automatically detected, specify Qt6_DIR:
   ```bash
   cmake .. -DQt6_DIR="C:/Qt/6.10.2/mingw_64/lib/cmake/Qt6"
   ```
   
   Or set environment variable:
   ```powershell
   $env:Qt6_DIR = "C:\Qt\6.10.2\mingw_64"
   cmake ..
   ```
   
   To build without GUI:
   ```bash
   cmake .. -DBUILD_GUI=OFF
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
.\scripts\build.bat debug test
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
├── backend/                      # Instrument transports (pure C++, no Qt)
│   ├── include/                  # IInstrumentConnection, TCP/IP, VISA, IEEE 488.2
│   └── core/                     # tcpip_connection, visa_connection, ieee4882, sockets
├── controller/                   # iio::InstrumentController bridge (no Qt)
│   ├── include/instrument_controller.h
│   └── instrument_controller.cpp
├── frontend/                     # User-facing apps (depend only on the controller)
│   ├── console/main.cpp          # Console application (interactiveIO)
│   ├── gui/                      # Qt6 GUI (interactiveIO-gui) + resources/ icon
│   └── api/                      # Command-line API (interactiveIO-api)
├── tests/                        # Google Test + Qt Test suites
│   ├── core/                     # backend tests
│   ├── controller/               # controller tests
│   ├── api/                      # Command-line API tests
│   └── gui/                      # GUI tests
├── scripts/                      # build.bat, build_debug.bat, package.ps1
├── docs/                         # Guides (deployment, VISA, USB, GUI, cross-platform)
├── cmake/iio_version.h.in        # Version header generated at configure time
├── CMakeLists.txt                # Top-level CMake configuration
├── CMakePresets.json             # mingw-qt preset
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

### GUI Application (interactiveIO-gui.exe) ⭐ Recommended

The Qt6-based GUI provides an intuitive interface for instrument communication:

1. **Launch the GUI:**
   ```powershell
   cd build\bin\Release
   .\interactiveIO-gui.exe
   ```

2. **Connect to an Instrument:**
   - Select protocol (TCP/IP Socket or VISA)
   - Click "Connect" button
   - Enter connection details in the dialog
   - Click "OK"

3. **Send Commands:**
   - Type SCPI command in the input field
   - Press Enter or click "Send"
   - View responses in the color-coded output window

**Features:**
- Modern dark mode interface with blue accents
- Color-coded output (commands, responses, errors)
- Timestamped messages with [HH:MM:SS] format
- Connection status indicator with visual feedback
- **VISA resource string helpers:**
  - TCP/IP VXI-11 (INSTR) helper
  - TCP/IP Raw Socket helper
  - GPIB helper with board/address inputs
  - **USB helper with Vendor/Product ID inputs**
- Command history navigation (Up/Down arrows)
- Auto-completion for previously used commands
- Clear output button
- Comprehensive tooltips for all controls
- Status bar with contextual messages


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