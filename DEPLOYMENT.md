# Deployment Guide - interactiveIO

## Overview

The interactiveIO application is designed to be easily deployed to end-user machines with minimal dependencies.

## Packaging for Distribution

### Automated Packaging

Use the provided script to create a deployment package:

```batch
package.bat
```

This will:
1. Build the application with static runtime linking (no DLL dependencies)
2. Create a distribution folder with all necessary files
3. Generate a ZIP archive for easy distribution
4. Include documentation and deployment instructions

### Manual Packaging

If you need to package manually:

1. **Build with static runtime:**
   ```batch
   cmake -B build -S . -DSTATIC_RUNTIME=ON
   cmake --build build --config Release
   ```

2. **Copy files:**
   ```
   dist/
   ├── interactiveIO.exe
   ├── DEPLOYMENT.txt
   └── docs/
       ├── README.md
       ├── VISA_GUIDE.md
       └── REFACTORING_SUMMARY.md
   ```

## Runtime Dependencies

### TCP/IP Connection (Built-in)
- ✅ **No additional dependencies**
- Uses Windows built-in Winsock2 (ws2_32.dll - part of Windows)
- Works on any Windows 7 or later system
- Fully self-contained

### VISA Connection (Optional)
- ⚠️ **Requires NI-VISA installation**
- VISA DLL (visa64.dll or visa32.dll) loaded dynamically at runtime
- If VISA is not installed:
  - Application still runs
  - TCP/IP connection available
  - VISA option gracefully indicates library not found
- Users download from: https://www.ni.com/visa

## Static vs Dynamic Runtime

### Static Runtime (Default)
- **Recommended for distribution**
- No vcruntime140.dll or msvcp140.dll required
- Larger executable (~75KB vs ~50KB)
- Self-contained - works on any Windows machine
- Enabled by default in package.bat

### Dynamic Runtime
- **For development only**
- Requires Visual C++ Redistributable installed
- Smaller executable
- To build with dynamic runtime:
  ```batch
  cmake -B build -S . -DSTATIC_RUNTIME=OFF
  cmake --build build --config Release
  ```

## Deployment Scenarios

### Scenario 1: TCP/IP Only Users
**Requirements:** None  
**Installation:**
1. Copy `interactiveIO.exe` to target machine
2. Run the application
3. Select TCP/IP option

**Result:** Works immediately, no setup required

### Scenario 2: VISA Users
**Requirements:** NI-VISA installed  
**Installation:**
1. Install NI-VISA on target machine (one time)
2. Copy `interactiveIO.exe` to target machine
3. Run the application
4. Select VISA option

**Result:** Full VISA functionality available

### Scenario 3: Mixed Environment
**Installation:**
1. Copy `interactiveIO.exe` to target machine
2. Application auto-detects VISA availability
3. Users see both options:
   - TCP/IP - always works
   - VISA - works if NI-VISA installed

**Result:** Automatic fallback to TCP/IP if VISA unavailable

## System Requirements

### Minimum Requirements
- **OS:** Windows 7 SP1 or later (Windows 10/11 recommended)
- **RAM:** 512 MB
- **Disk:** 10 MB free space
- **Network:** Ethernet adapter (for TCP/IP connections)

### Recommended Requirements
- **OS:** Windows 10 21H2 or later
- **RAM:** 2 GB
- **For VISA:** NI-VISA 20.0 or later

## Verification Steps

After packaging, verify the deployment:

1. **Clean Test Machine**
   - Use a Windows VM or clean machine without development tools
   - Ensure no Visual Studio installed

2. **Copy Package**
   - Copy the entire dist folder or ZIP file
   - Extract if using ZIP

3. **Test TCP/IP**
   ```batch
   cd dist\interactiveIO-v1.0.0-Windows-x64
   interactiveIO.exe
   ```
   - Select option 1 (TCP/IP)
   - Should work immediately

4. **Test VISA (if available)**
   - Install NI-VISA first
   - Run interactiveIO.exe
   - Select option 2 (VISA)
   - Should detect VISA successfully

## Troubleshooting

### "Application failed to start" or missing DLL errors

**Cause:** Built with dynamic runtime (/MD)  
**Solution:** Rebuild with static runtime:
```batch
cmake -B build -S . -DSTATIC_RUNTIME=ON
cmake --build build --config Release
```

### VISA option shows "not available"

**Cause:** NI-VISA not installed or wrong architecture (32 vs 64 bit)  
**Solution:**
1. Install NI-VISA from https://www.ni.com/visa
2. Match architecture (use 64-bit VISA for 64-bit app)
3. Restart application

### TCP/IP connection fails

**Causes:**
- Firewall blocking
- Wrong IP/port
- Instrument not responding

**Solution:**
- Check Windows Firewall settings
- Verify instrument IP address and port
- Test with ping first

## License Considerations

### Your Application
- interactiveIO.exe can be freely distributed
- Include documentation with proper attribution

### Third-Party Components
- **NI-VISA**: End users must accept NI license when installing
- **VISA DLLs**: Cannot be redistributed - users must install NI-VISA
- **Windows APIs**: Part of Windows OS, no separate license needed

## Distribution Checklist

Before distributing:

- [ ] Built with STATIC_RUNTIME=ON
- [ ] Tested on clean Windows machine
- [ ] Verified TCP/IP functionality
- [ ] Verified VISA functionality (if VISA installed)
- [ ] Included all documentation
- [ ] Created DEPLOYMENT.txt or similar user guide
- [ ] Version number updated in CMakeLists.txt
- [ ] ZIP archive created with package.bat
- [ ] README.md included in package
- [ ] Contact/support information provided

## Network Deployment

For corporate environments deploying to multiple machines:

### Silent Installation (PowerShell)
```powershell
# Copy to Program Files
$DestPath = "$env:ProgramFiles\interactiveIO"
New-Item -ItemType Directory -Force -Path $DestPath
Copy-Item "interactiveIO.exe" -Destination $DestPath

# Create Start Menu shortcut
$WshShell = New-Object -ComObject WScript.Shell
$Shortcut = $WshShell.CreateShortcut(
    "$env:ProgramData\Microsoft\Windows\Start Menu\Programs\interactiveIO.lnk"
)
$Shortcut.TargetPath = "$DestPath\interactiveIO.exe"
$Shortcut.Save()
```

### Group Policy Deployment
1. Place package on network share
2. Create GPO for software installation
3. Deploy as "Published" or "Assigned"

## Updates

To deploy updates:

1. Build new version with updated VERSION in CMakeLists.txt
2. Run package.bat
3. Distribute new ZIP or exe
4. Users can simply replace old exe with new one

Configuration persists (user settings in registry/config file if implemented).

## Support

For deployment issues:
- Check build logs in `build/` folder
- Review CMake configuration output
- Test on clean Windows VM
- Consult README.md for usage help
- Visit: https://github.com/haziq-zur/interactiveIO

---

**Created:** 2026  
**Version:** 1.0.0  
**Maintained by:** haziq-zur
