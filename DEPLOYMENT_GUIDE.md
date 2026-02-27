# Qt Deployment Guide for InteractiveIO

## Problem: Missing Qt DLL Error

When running `interactiveIO-gui.exe`, you may encounter an error:
```
The code execution cannot proceed because Qt6Core.dll was not found.
```

This happens because Qt runtime libraries aren't in the executable's directory or system PATH.

## Quick Solution

### Option 1: Automatic Deployment (Recommended)
The build script now automatically deploys Qt dependencies. Just rebuild:

```bash
build.bat release
```

The GUI executable with all dependencies will be in `build\bin\Release\`

### Option 2: Manual Deployment
If you've already built the project, run the deployment script:

```bash
deploy.bat
```

This copies all necessary Qt DLLs to the executable directory.

### Option 3: Create Distributable Package
To create a standalone package you can share:

```bash
deploy.bat package
```

This creates a `deploy\` folder with everything needed to run on other computers.

## What Gets Deployed

The deployment process copies:

### Core Qt Libraries (~34 MB)
- `Qt6Core.dll` - Core functionality
- `Qt6Gui.dll` - GUI components
- `Qt6Widgets.dll` - Widget toolkit
- `Qt6Network.dll` - Network support
- `Qt6Svg.dll` - SVG icon support

### MinGW Runtime (~2.4 MB)
- `libgcc_s_seh-1.dll`
- `libstdc++-6.dll`
- `libwinpthread-1.dll`

### Graphics Support (~24 MB)
- `opengl32sw.dll` - Software OpenGL renderer
- `D3Dcompiler_47.dll` - DirectX shader compiler

### Qt Plugins (~5 MB)
- `platforms\qwindows.dll` - Windows platform integration
- `styles\qmodernwindowsstyle.dll` - Modern Windows styling
- `iconengines\qsvgicon.dll` - SVG icon rendering
- `imageformats\*.dll` - Image format support (JPEG, PNG, GIF, SVG)
- `networkinformation\qnetworklistmanager.dll` - Network detection
- `tls\*.dll` - Secure connections

**Total Size:** ~65 MB

## Distribution

### For End Users
Distribute the entire `build\bin\Release\` folder or use `deploy.bat package` to create a clean distribution package.

### System Requirements
- Windows 10 or later (x64)
- No Qt installation required on target machines
- No Visual C++ Redistributables needed (MinGW runtime included)

## Troubleshooting

### "Application failed to start" error
Run deployment again:
```bash
deploy.bat
```

### Missing specific plugin DLL
Ensure you're running from the `build\bin\Release\` directory, not copying just the .exe file.

### Different Qt Version
If using a different Qt installation, update paths in:
- `build.bat` (line ~55)
- `deploy.bat` (line ~10)

Change:
```batch
set QT_PATH=C:\Qt\6.10.2\mingw_64
```

To your Qt installation path.

## Advanced: Static Linking

For a single-file executable without DLL dependencies, you need to:
1. Build Qt statically (time-consuming)
2. Configure project with static linking
3. Rebuild with `-DSTATIC_RUNTIME=ON`

This is not recommended for development but useful for minimal deployment.

## Automated Deployment in CMake

You can also add post-build deployment in CMakeLists.txt:

```cmake
if(WIN32 AND Qt6_FOUND)
    add_custom_command(TARGET interactiveIO-gui POST_BUILD
        COMMAND ${Qt6_DIR}/../../../bin/windeployqt.exe
            --release --no-translations $<TARGET_FILE:interactiveIO-gui>
        COMMENT "Deploying Qt dependencies..."
    )
endif()
```

This automatically runs `windeployqt` after each build.

## Summary

✅ **After first build:** Qt DLLs automatically deployed  
✅ **Can run directly:** Just double-click `interactiveIO-gui.exe`  
✅ **Portable:** Copy entire `build\bin\Release\` folder to any Windows PC  
✅ **No installation:** No Qt or Visual C++ required on target machines
