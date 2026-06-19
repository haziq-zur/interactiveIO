@echo off
REM Package interactiveIO application for deployment
REM Creates a standalone distribution package
REM Run from the repository root regardless of where the script is invoked from
pushd "%~dp0.."

setlocal

set VERSION=1.0.0
set PACKAGE_NAME=interactiveIO-v%VERSION%-Windows-x64
set PACKAGE_DIR=.\dist\%PACKAGE_NAME%

echo =============================================
echo  Packaging interactiveIO v%VERSION%
echo =============================================
echo.

REM Clean and create package directory
if exist .\dist rmdir /s /q .\dist
mkdir "%PACKAGE_DIR%"
mkdir "%PACKAGE_DIR%\docs"

REM Build Release version with static runtime
echo Building Release version with static runtime...
cmake -B build -S . -DCMAKE_BUILD_TYPE=Release -DSTATIC_RUNTIME=ON
if errorlevel 1 (
    echo Build configuration failed!
    exit /b 1
)

cmake --build build --config Release
if errorlevel 1 (
    echo Build failed!
    exit /b 1
)

echo.
echo Copying files to package directory...

REM Copy executable
copy /Y build\bin\Release\interactiveIO.exe "%PACKAGE_DIR%\"
if errorlevel 1 (
    echo Failed to copy executable!
    exit /b 1
)

REM Copy documentation
copy /Y README.md "%PACKAGE_DIR%\docs\"
copy /Y docs\VISA_GUIDE.md "%PACKAGE_DIR%\docs\" 2>nul
copy /Y docs\USB_GUIDE.md "%PACKAGE_DIR%\docs\" 2>nul

REM Create deployment info file
echo Creating DEPLOYMENT.txt...
(
echo =============================================
echo  interactiveIO v%VERSION%
echo  Interactive Instrument Communication Tool
echo =============================================
echo.
echo CONTENTS:
echo   - interactiveIO.exe      Main application
echo   - docs\README.md         User documentation
echo   - docs\VISA_GUIDE.md     VISA setup guide
echo   - DEPLOYMENT.txt         This file
echo.
echo REQUIREMENTS:
echo   - Windows 7 or later
echo   - No additional runtime dependencies for TCP/IP
echo   - NI-VISA required for VISA protocol support ^(optional^)
echo     Download from: https://www.ni.com/visa
echo.
echo QUICK START:
echo   1. Run interactiveIO.exe
echo   2. Select protocol ^(TCP/IP or VISA^)
echo   3. Configure connection
echo   4. Send SCPI commands
echo.
echo TCP/IP CONNECTION:
echo   - Works out of the box
echo   - No additional software required
echo   - Direct socket communication
echo.
echo VISA CONNECTION:
echo   - Requires NI-VISA installation
echo   - Supports GPIB, USB, VXI-11, Serial
echo   - DLL loaded dynamically at runtime
echo   - Application works without VISA ^(TCP/IP still available^)
echo.
echo DEPLOYMENT:
echo   This package is self-contained for TCP/IP connections.
echo   Simply copy the interactiveIO.exe to any Windows machine.
echo.
echo   For VISA support, users must install NI-VISA separately:
echo   https://www.ni.com/en-us/support/downloads/drivers/download.ni-visa.html
echo.
echo SUPPORT:
echo   - Documentation: docs\README.md
echo   - VISA Guide: docs\VISA_GUIDE.md
echo   - GitHub: https://github.com/haziq-zur/interactiveIO
echo.
echo BUILD INFO:
echo   - Version: %VERSION%
echo   - Build Date: %DATE% %TIME%
echo   - Configuration: Release ^(Static Runtime^)
echo   - Platform: Windows x64
echo   - Compiler: MSVC
echo.
) > "%PACKAGE_DIR%\DEPLOYMENT.txt"

REM Create a simple launcher script
echo Creating launcher script...
(
echo @echo off
echo REM Simple launcher for interactiveIO
echo REM This can be customized with default settings
echo.
echo REM Uncomment to set default IP and port ^(for automation^)
echo REM set INTERACTIVEIO_IP=192.168.1.100
echo REM set INTERACTIVEIO_PORT=5025
echo.
echo start /wait interactiveIO.exe
) > "%PACKAGE_DIR%\run.bat"

echo.
echo Package created successfully at: .\dist\%PACKAGE_NAME%
echo.

REM Create ZIP archive if 7-Zip or PowerShell is available
where 7z >nul 2>&1
if not errorlevel 1 (
    echo Creating ZIP archive with 7-Zip...
    cd dist
    7z a -tzip "%PACKAGE_NAME%.zip" "%PACKAGE_NAME%\*" -r
    cd ..
    echo Archive created: .\dist\%PACKAGE_NAME%.zip
) else (
    echo Creating ZIP archive with PowerShell...
    powershell -Command "Compress-Archive -Path '.\dist\%PACKAGE_NAME%' -DestinationPath '.\dist\%PACKAGE_NAME%.zip' -Force"
    if not errorlevel 1 (
        echo Archive created: .\dist\%PACKAGE_NAME%.zip
    ) else (
        echo ZIP creation failed. Manual packaging required.
    )
)

echo.
echo =============================================
echo  Packaging Complete!
echo =============================================
echo.
echo Distribution files:
dir /b .\dist
echo.
echo You can now distribute the ZIP file or the folder.

endlocal
popd
