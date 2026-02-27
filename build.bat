@echo off
SETLOCAL ENABLEDELAYEDEXPANSION
REM Build script for interactiveIO using CMake

echo ====================================
echo Building interactiveIO with CMake
echo ====================================
echo.

REM Parse command line arguments
set BUILD_TYPE=Release
set RUN_TESTS=0

:parse_args
if "%~1"=="" goto end_parse
if /i "%~1"=="debug" set BUILD_TYPE=Debug
if /i "%~1"=="release" set BUILD_TYPE=Release
if /i "%~1"=="test" set RUN_TESTS=1
shift
goto parse_args
:end_parse

echo Build Type: %BUILD_TYPE%
echo.

REM Create build directory if it doesn't exist
if not exist "build" (
    echo Creating build directory...
    mkdir build
)

REM Configure and build
echo Configuring CMake...
cmake -B build -S . -DCMAKE_BUILD_TYPE=%BUILD_TYPE%
if errorlevel 1 (
    echo CMake configuration failed!
    exit /b 1
)

echo.
echo Building project...
cmake --build build --config %BUILD_TYPE%
if errorlevel 1 (
    echo Build failed!
    exit /b 1
)

echo.
echo ====================================
echo Build completed successfully!
echo ====================================
echo Build Type: %BUILD_TYPE%
echo Executable location: build\bin\%BUILD_TYPE%\interactiveIO.exe
echo.

REM Deploy Qt dependencies for GUI if it exists
if exist "build\bin\%BUILD_TYPE%\interactiveIO-gui.exe" (
    echo Deploying Qt dependencies for GUI...
    set QT_PATH=C:\Qt\6.10.2\mingw_64
    if exist "!QT_PATH!\bin\windeployqt.exe" (
        "!QT_PATH!\bin\windeployqt.exe" --release --no-translations "build\bin\%BUILD_TYPE%\interactiveIO-gui.exe" >nul 2>&1
        if errorlevel 1 (
            echo Warning: Failed to deploy Qt dependencies. Run deploy.bat manually.
        ) else (
            echo ✓ Qt dependencies deployed successfully
            echo GUI location: build\bin\%BUILD_TYPE%\interactiveIO-gui.exe
        )
    ) else (
        echo Warning: windeployqt not found. Run deploy.bat to copy Qt DLLs.
    )
    echo.
)

REM Run tests if requested
if %RUN_TESTS%==1 (
    echo.
    echo ====================================
    echo Running Unit Tests
    echo ====================================
    echo.
    cd build
    ctest -C %BUILD_TYPE% --output-on-failure
    if errorlevel 1 (
        echo.
        echo Tests failed!
        cd ..
        exit /b 1
    )
    cd ..
    echo.
    echo ====================================
    echo All tests passed!
    echo ====================================
)

echo.
echo Usage:
echo   build.bat [release^|debug] [test]
echo.
echo Examples:
echo   build.bat              - Build in Release mode
echo   build.bat debug        - Build in Debug mode
echo   build.bat release test - Build in Release mode and run tests
echo   build.bat debug test   - Build in Debug mode and run tests
echo.
echo To run the program:
echo   cd build\bin\%BUILD_TYPE%
echo   interactiveIO.exe
echo.

pause
