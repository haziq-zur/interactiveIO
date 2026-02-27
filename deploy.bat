@echo off
REM Deployment script for interactiveIO GUI application
REM This script copies Qt dependencies and creates a distributable package

echo ====================================
echo Deploying interactiveIO GUI
echo ====================================
echo.

set BUILD_DIR=build\bin\Release
set DEPLOY_DIR=deploy
set QT_PATH=C:\Qt\6.10.2\mingw_64

REM Check if executable exists
if not exist "%BUILD_DIR%\interactiveIO-gui.exe" (
    echo ERROR: interactiveIO-gui.exe not found!
    echo Please build the project first using build.bat
    exit /b 1
)

echo Step 1: Running windeployqt to copy Qt dependencies...
"%QT_PATH%\bin\windeployqt.exe" --release --no-translations "%BUILD_DIR%\interactiveIO-gui.exe"
if errorlevel 1 (
    echo ERROR: windeployqt failed!
    exit /b 1
)

echo.
echo Step 2: Verifying deployment...
if not exist "%BUILD_DIR%\Qt6Core.dll" (
    echo ERROR: Qt6Core.dll not found after deployment!
    exit /b 1
)

echo ✓ Qt6Core.dll found
echo ✓ All dependencies deployed successfully!
echo.

REM Optional: Create distributable package
if "%1"=="package" (
    echo Step 3: Creating deployment package...
    
    if not exist "%DEPLOY_DIR%" mkdir "%DEPLOY_DIR%"
    
    echo Copying files to %DEPLOY_DIR%...
    xcopy /E /I /Y "%BUILD_DIR%\*" "%DEPLOY_DIR%"
    
    REM Add documentation
    copy README.md "%DEPLOY_DIR%\" >nul 2>&1
    copy VISA_GUIDE.md "%DEPLOY_DIR%\" >nul 2>&1
    
    echo.
    echo ✓ Deployment package created in %DEPLOY_DIR%\
    echo   You can distribute this folder as a standalone application.
)

echo.
echo ====================================
echo Deployment completed successfully!
echo ====================================
echo.
echo Application location: %BUILD_DIR%\interactiveIO-gui.exe
echo.
echo To create a distributable package, run:
echo   deploy.bat package
echo.
pause
