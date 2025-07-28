@echo off
echo Installing PS5 Camera Auto-Loader Service...

:: Check for administrative privileges
net session >nul 2>&1
if %errorlevel% neq 0 (
    echo Administrative privileges required. Please run as administrator.
    pause
    exit /b 1
)

echo Administrative privileges confirmed.

:: Get current directory (where the batch file is located)
set "CURRENT_DIR=%~dp0"

:: Remove trailing backslash if present
if "%CURRENT_DIR:~-1%"=="\" set "CURRENT_DIR=%CURRENT_DIR:~0,-1%"

:: Define service name
set "SERVICE_NAME=PS5CameraService"

:: Define full paths
set "SERVICE_EXE=%CURRENT_DIR%\PS5CameraService.exe"
set "LOADER_EXE=%CURRENT_DIR%\PS5CameraLoader.exe"
set "FIRMWARE_FILE=%CURRENT_DIR%\firmware_discord_and_gamma_fix.bin"

:: Check if required files exist
if not exist "%SERVICE_EXE%" (
    echo Error: PS5CameraService.exe not found in current directory.
    pause
    exit /b 1
)

if not exist "%LOADER_EXE%" (
    echo Error: PS5CameraLoader.exe not found in current directory.
    pause
    exit /b 1
)

if not exist "%FIRMWARE_FILE%" (
    echo Error: firmware_discord_and_gamma_fix.bin not found in current directory.
    pause
    exit /b 1
)

echo All required files found.

:: Check if service already exists
sc query "%SERVICE_NAME%" >nul 2>&1
if %errorlevel% equ 0 (
    echo Service already exists. Stopping and removing existing service...
    
    :: Stop the service if it's running
    sc stop "%SERVICE_NAME%" >nul 2>&1
    if %errorlevel% equ 0 (
        echo Service stopped successfully.
    ) else (
        echo Service was not running or failed to stop.
    )
    
    :: Wait a moment for service to fully stop
    timeout /t 2 /nobreak >nul
    
    :: Delete the existing service
    sc delete "%SERVICE_NAME%" >nul 2>&1
    if %errorlevel% equ 0 (
        echo Existing service removed successfully.
    ) else (
        echo Failed to remove existing service.
        pause
        exit /b 1
    )
    
    :: Wait a moment for service to be fully removed
    timeout /t 2 /nobreak >nul
)

:: Create the new service with full path
echo Creating new service...
sc create "%SERVICE_NAME%" binPath= "\"%SERVICE_EXE%\"" DisplayName= "PS5 Camera Auto-Loader Service" start= auto obj= LocalSystem
if %errorlevel% neq 0 (
    echo Failed to create service.
    pause
    exit /b 1
)

:: Set service description
sc description "%SERVICE_NAME%" "Automatically loads firmware for PS5 Camera when connected"

:: Start the service
echo Starting service...
sc start "%SERVICE_NAME%"
if %errorlevel% neq 0 (
    echo Failed to start service. Check Windows Event Viewer for details.
    echo Service created but not started. You can try starting it manually.
    pause
    exit /b 1
)

echo.
echo Service installed and started successfully!
echo PS5 Camera will now automatically load firmware when connected.
echo.
echo Service Name: %SERVICE_NAME%
echo Service Path: %SERVICE_EXE%
echo.
echo To uninstall, run: sc stop "%SERVICE_NAME%" && sc delete "%SERVICE_NAME%"
echo.
pause
