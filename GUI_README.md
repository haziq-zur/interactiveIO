# Qt6 GUI Implementation

## Overview
A modern Qt6-based GUI has been implemented for the interactiveIO instrument communication tool. The GUI provides an intuitive interface for connecting to instruments via TCP/IP or VISA and sending SCPI commands.

## Features

### Main Window
- **Protocol Selection**: Choose between TCP/IP Socket and VISA protocols
- **Connection Status**: Visual indicator showing connection state
- **Command Input**: Text field for entering SCPI commands
- **Output Display**: Timestamped, color-coded output showing sent commands and responses
- **Real-time Feedback**: Immediate visual feedback for all operations

### Connection Dialog
- **TCP/IP Setup**:
  - IP address input with validation
  - Port number configuration (default: 5025)
  
- **VISA Setup**:
  - Direct resource string input
  - Quick helpers for common resource formats:
    - VXI-11 (INSTR): `TCPIP::IP::INSTR`
    - Raw Socket: `TCPIP::IP::PORT::SOCKET`
    - GPIB: `GPIB0::ADDRESS::INSTR`
  - Example resource strings provided

### User Interface Features
- **Color-coded output**:
  - Blue: System messages
  - Dark blue: Sent commands (>>)
  - Dark green: Received responses (<<)
  - Red: Errors
  - Orange: Warnings
  - Gray: Informational messages
- **Timestamp**: All messages include HH:MM:SS timestamp
- **Clear Output**: Button to clear output window
- **Status Bar**: Shows connection status and messages

## File Structure

```
gui/
├── main_gui.cpp           # Qt application entry point
├── mainwindow.h           # Main window class header
├── mainwindow.cpp         # Main window implementation
├── mainwindow.ui          # Main window Qt Designer UI file
├── connectiondialog.h     # Connection dialog header
├── connectiondialog.cpp   # Connection dialog implementation
├── connectiondialog.ui    # Connection dialog Qt Designer UI file
└── resources.qrc          # Qt resource file (for future icons/images)
```

## Building

### Prerequisites
1. **Qt6 Installation** (6.6.1 or later recommended):
   - Download from: https://www.qt.io/download-qt-installer
   - Install with MSVC 2022 64-bit components
   - Note the installation path (e.g., `C:\Qt\6.6.1\msvc2019_64`)

2. **CMake 3.15+**
3. **Visual Studio 2022** (or compatible compiler)

### Build Commands

#### Option 1: Automatic Qt6 Detection
If Qt6 is in your PATH:
```powershell
# Configure
cmake -B build -S . -DCMAKE_BUILD_TYPE=Release

# Build
cmake --build build --config Release

# Output: build/bin/Release/interactiveIO-gui.exe
```

#### Option 2: Specify Qt6 Path
If Qt6 is not in PATH:
```powershell
# Set Qt6 path
$env:Qt6_DIR = "C:\Qt\6.6.1\msvc2019_64"

# Or specify during configuration
cmake -B build -S . -DCMAKE_BUILD_TYPE=Release -DQt6_DIR="C:\Qt\6.6.1\msvc2019_64\lib\cmake\Qt6"

# Build
cmake --build build --config Release
```

#### Option 3: Build Console Only (without Qt6)
```powershell
cmake -B build -S . -DCMAKE_BUILD_TYPE=Release -DBUILD_GUI=OFF
cmake --build build --config Release
```

### Build Outputs
- **Console application**: `build/bin/Release/interactiveIO.exe`
- **GUI application**: `build/bin/Release/interactiveIO-gui.exe`

## Usage

### Starting the GUI
```powershell
cd build\bin\Release
.\interactiveIO-gui.exe
```

### Connecting to an Instrument

#### TCP/IP Connection:
1. Select "TCP/IP Socket" from the Protocol dropdown
2. Click "Connect" button
3. Enter IP address (e.g., 192.168.1.100)
4. Enter port number (e.g., 5025)
5. Click "OK"

#### VISA Connection:
1. Select "VISA" from the Protocol dropdown
2. Click "Connect" button
3. Enter resource string directly, or use quick helpers:
   - For VXI-11: Enter IP, click "VXI-11 (INSTR)"
   - For Raw Socket: Enter IP and port, click "Raw Socket"
   - For GPIB: Enter board and address, click "GPIB"
4. Click "OK"

### Sending Commands
1. Ensure connected to an instrument
2. Type SCPI command in the command input field
3. Press Enter or click "Send" button
4. For queries (containing '?'), response will be displayed automatically
5. Command history appears in the output window

### Example Commands
```
*IDN?                    # Query instrument identification
*RST                     # Reset instrument
:MEAS:VOLT?              # Measure voltage
:SYST:ERR?               # Query system errors
:CONF:VOLT:DC 10,0.001  # Configure voltage measurement
```

## Architecture

### Design Pattern
The GUI follows the Model-View-Controller (MVC) pattern:
- **Model**: `IInstrumentConnection` interface and implementations
- **View**: Qt UI files (.ui) designed with Qt Designer
- **Controller**: MainWindow and ConnectionDialog classes

### Key Components

#### IInstrumentConnection Interface
The existing interface-based design works perfectly with Qt:
```cpp
class IInstrumentConnection {
    virtual bool connect(const std::string& address, int port = 0) = 0;
    virtual bool sendCommand(const std::string& command) = 0;
    virtual std::string readResponse(int timeoutSeconds = 10) = 0;
    // ... other methods
};
```

#### MainWindow
Manages the main application window:
- Connection management
- Command sending/receiving
- Output formatting
- Status updates

#### ConnectionDialog
Handles connection configuration:
- Protocol-specific inputs
- Input validation
- Helper functions for common configurations

## Customization

### Modifying the UI
1. Open `.ui` files in Qt Designer or Qt Creator
2. Edit layout, widgets, properties visually
3. Save and rebuild - changes are automatic via CMake's AUTOUIC

### Adding Icons
1. Add icon files to `gui/` directory
2. Update `resources.qrc`:
   ```xml
   <qresource prefix="/">
       <file>icons/connect.png</file>
       <file>icons/disconnect.png</file>
   </qresource>
   ```
3. Use in code: `QIcon(":/icons/connect.png")`

### Styling
Modify the appearance using Qt Style Sheets (CSS-like):
```cpp
ui->outputText->setStyleSheet(
    "QTextEdit { background-color: #1e1e1e; color: #ffffff; }"
);
```

## Deployment

### Windows Deployment
Use Qt's `windeployqt` tool to package dependencies:
```powershell
cd build\bin\Release
C:\Qt\6.6.1\msvc2019_64\bin\windeployqt.exe interactiveIO-gui.exe
```

This copies all required Qt DLLs to the executable directory.

### Creating Installer
See [PACKAGING_QUICK_REF.md](../PACKAGING_QUICK_REF.md) for creating installers with CPack.

## Troubleshooting

### Qt6 Not Found
**Error**: `Qt6 not found - GUI application will not be built`

**Solutions**:
1. Install Qt6 from https://www.qt.io/download-qt-installer
2. Set Qt6_DIR environment variable:
   ```powershell
   $env:Qt6_DIR = "C:\Qt\6.6.1\msvc2019_64"
   ```
3. Or specify in CMake:
   ```powershell
   cmake -DQt6_DIR="C:\Qt\6.6.1\msvc2019_64\lib\cmake\Qt6" ...
   ```

### Missing Qt DLLs at Runtime
**Error**: Application fails to start, missing Qt6Core.dll, etc.

**Solutions**:
1. Run `windeployqt` (see Deployment section)
2. Or add Qt bin directory to PATH:
   ```powershell
   $env:PATH = "C:\Qt\6.6.1\msvc2019_64\bin;$env:PATH"
   ```

### VISA Library Not Available
**Error**: "VISA library not found!"

**Solution**: Install NI-VISA from https://www.ni.com/visa

### Connection Timeout
- Check instrument IP address/resource string
- Verify network connectivity
- Ensure instrument is powered on
- Check firewall settings
- Verify port number (common: 5025, 5024)

## Future Enhancements

Potential features to add:
- [ ] Command history with up/down arrow navigation
- [ ] Save/load command scripts
- [ ] Multiple instrument connections (tabs)
- [ ] Response data plotting
- [ ] Instrument-specific profiles
- [ ] Dark/light theme toggle
- [ ] Export output to file
- [ ] Search in output
- [ ] Custom toolbar for common commands

## References

- Qt6 Documentation: https://doc.qt.io/qt-6/
- Qt Designer: https://doc.qt.io/qt-6/qtdesigner-manual.html
- CMake Qt Integration: https://cmake.org/cmake/help/latest/manual/cmake-qt.7.html
- SCPI Standard: https://www.ivifoundation.org/scpi/

## Support

For issues or questions:
1. Check [QT6_MIGRATION_PLAN.md](../QT6_MIGRATION_PLAN.md) for detailed migration info
2. Review [VISA_GUIDE.md](../VISA_GUIDE.md) for VISA-specific help
3. Check [README.md](../README.md) for project overview
