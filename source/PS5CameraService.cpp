#include "pch.h"
#include "OrbisEyeCam.h"
#include "ServiceGUIDs.h"
#include <windows.h>
#include <winsvc.h>
#include <dbt.h>
#include <thread>
#include <chrono>
#include <iostream>
#include <fstream>

using namespace orbiseye;

class PS5CameraService {
private:
    SERVICE_STATUS serviceStatus;
    SERVICE_STATUS_HANDLE serviceStatusHandle;
    HWND messageWindow;
    bool isRunning;
    std::thread deviceThread;
    
public:
    static PS5CameraService* instance;
    
    PS5CameraService() : messageWindow(NULL), isRunning(false) {
        serviceStatus = {};
        serviceStatusHandle = NULL;
    }
    
    static void WINAPI ServiceMain(DWORD argc, LPTSTR* argv) {
        instance = new PS5CameraService();
        instance->Run();
    }
    
    static void WINAPI ServiceCtrlHandler(DWORD opcode) {
        switch (opcode) {
        case SERVICE_CONTROL_STOP:
            if (instance) {
                instance->Stop();
            }
            break;
        case SERVICE_CONTROL_INTERROGATE:
            SetServiceStatus(instance->serviceStatusHandle, &instance->serviceStatus);
            break;
        }
    }
    
    void Run() {
        // Register service control handler
        serviceStatusHandle = RegisterServiceCtrlHandler(L"PS5CameraService", ServiceCtrlHandler);
        if (serviceStatusHandle == 0) {
            WriteToEventLog(L"RegisterServiceCtrlHandler failed");
            return;
        }
        
        // Set service status to starting
        serviceStatus.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
        serviceStatus.dwCurrentState = SERVICE_START_PENDING;
        serviceStatus.dwControlsAccepted = SERVICE_ACCEPT_STOP;
        serviceStatus.dwWin32ExitCode = 0;
        serviceStatus.dwServiceSpecificExitCode = 0;
        serviceStatus.dwCheckPoint = 0;
        serviceStatus.dwWaitHint = 0;
        SetServiceStatus(serviceStatusHandle, &serviceStatus);
        
        // Initialize service
        if (!Initialize()) {
            serviceStatus.dwCurrentState = SERVICE_STOPPED;
            serviceStatus.dwWin32ExitCode = GetLastError();
            SetServiceStatus(serviceStatusHandle, &serviceStatus);
            return;
        }
        
        // Set service status to running
        serviceStatus.dwCurrentState = SERVICE_RUNNING;
        SetServiceStatus(serviceStatusHandle, &serviceStatus);
        
        WriteToEventLog(L"PS5 Camera Service started successfully");
        
        // Start device monitoring
        StartDeviceMonitoring();
        
        // Message loop
        MSG msg;
        while (isRunning && GetMessage(&msg, NULL, 0, 0)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        
        // Cleanup
        Cleanup();
    }
    
    bool Initialize() {
        // Create message-only window for device notifications
        WNDCLASS wc = {};
        wc.lpfnWndProc = WindowProc;
        wc.hInstance = GetModuleHandle(NULL);
        wc.lpszClassName = L"PS5CameraServiceWindow";
        
        if (!RegisterClass(&wc)) {
            WriteToEventLog(L"Failed to register window class");
            return false;
        }
        
        messageWindow = CreateWindow(L"PS5CameraServiceWindow", L"", 0, 
                                   0, 0, 0, 0, HWND_MESSAGE, NULL, 
                                   GetModuleHandle(NULL), this);
        
        if (!messageWindow) {
            WriteToEventLog(L"Failed to create message window");
            return false;
        }
        
        SetWindowLongPtr(messageWindow, GWLP_USERDATA, (LONG_PTR)this);
        
        // Register for device notifications
        DEV_BROADCAST_DEVICEINTERFACE filter = {};
        filter.dbcc_size = sizeof(filter);
        filter.dbcc_devicetype = DBT_DEVTYP_DEVICEINTERFACE;
        filter.dbcc_classguid = GUID_DEVINTERFACE_USB_DEVICE;
        
        HDEVNOTIFY hDevNotify = RegisterDeviceNotification(messageWindow, &filter, 
                                                          DEVICE_NOTIFY_WINDOW_HANDLE);
        
        if (!hDevNotify) {
            WriteToEventLog(L"Failed to register device notification");
            return false;
        }
        
        isRunning = true;
        return true;
    }
    
    void StartDeviceMonitoring() {
        // Check for existing devices on startup
        CheckAndUploadFirmware();
        
        // Start background monitoring thread
        deviceThread = std::thread([this]() {
            while (isRunning) {
                std::this_thread::sleep_for(std::chrono::seconds(10));
                // Periodic check for devices that might have been missed
                CheckAndUploadFirmware();
            }
        });
    }
    
    static LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
        PS5CameraService* service = (PS5CameraService*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
        
        if (msg == WM_DEVICECHANGE && service) {
            if (wParam == DBT_DEVICEARRIVAL) {
                // Device connected - check if it's PS5 camera
                std::thread([service]() {
                    // Wait a moment for device to be ready
                    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
                    service->CheckAndUploadFirmware();
                }).detach();
            }
        }
        
        return DefWindowProc(hwnd, msg, wParam, lParam);
    }
    
    void CheckAndUploadFirmware() {
        try {
            auto devices = OrbisEyeCam::getDevices(true);
            if (!devices.empty()) {
                WriteToEventLog(L"PS5 Camera detected in boot mode");
                
                for (auto& device : devices) {
                    std::string firmwarePath = GetFirmwarePath();
                    
                    // Check if firmware file exists
                    std::ifstream file(firmwarePath);
                    if (!file.good()) {
                        WriteToEventLog(L"Firmware file not found: firmware_discord_and_gamma_fix.bin");
                        continue;
                    }
                    file.close();
                    
                    device->firmware_path = firmwarePath;
                    device->firmware_upload();
                    
                    WriteToEventLog(L"Firmware uploaded successfully");
                }
            }
        }
        catch (const std::exception& e) {
            std::wstring error = L"Error checking devices: ";
            error += std::wstring(e.what(), e.what() + strlen(e.what()));
            WriteToEventLog(error.c_str());
        }
    }
    
    std::string GetFirmwarePath() {
        char path[MAX_PATH];
        GetModuleFileNameA(NULL, path, MAX_PATH);
        std::string exePath(path);
        size_t pos = exePath.find_last_of("\\/");
        return exePath.substr(0, pos + 1) + "firmware_discord_and_gamma_fix.bin";
    }
    
    void WriteToEventLog(const wchar_t* message) {
        HANDLE hEventSource = RegisterEventSource(NULL, L"PS5CameraService");
        if (hEventSource) {
            const wchar_t* strings[1] = { message };
            ReportEvent(hEventSource, EVENTLOG_INFORMATION_TYPE, 0, 0, NULL, 1, 0, strings, NULL);
            DeregisterEventSource(hEventSource);
        }
        
        // Also output to console for debugging
        std::wcout << L"PS5CameraService: " << message << std::endl;
    }
    
    void Stop() {
        WriteToEventLog(L"PS5 Camera Service stopping...");
        
        isRunning = false;
        
        if (deviceThread.joinable()) {
            deviceThread.join();
        }
        
        serviceStatus.dwCurrentState = SERVICE_STOPPED;
        SetServiceStatus(serviceStatusHandle, &serviceStatus);
        
        PostQuitMessage(0);
    }
    
    void Cleanup() {
        if (messageWindow) {
            DestroyWindow(messageWindow);
            messageWindow = NULL;
        }
    }
};

PS5CameraService* PS5CameraService::instance = nullptr;

// Service entry point
int main(int argc, char* argv[]) {
    // Check if running as console application for testing
    if (argc > 1 && strcmp(argv[1], "--console") == 0) {
        std::cout << "Running PS5 Camera Service in console mode..." << std::endl;
        
        PS5CameraService service;
        if (service.Initialize()) {
            service.StartDeviceMonitoring();
            
            std::cout << "Press Enter to stop..." << std::endl;
            std::cin.get();
            
            service.Stop();
        }
        return 0;
    }
    
    // Run as Windows Service
    SERVICE_TABLE_ENTRY serviceTable[] = {
        { (LPWSTR)L"PS5CameraService", PS5CameraService::ServiceMain },
        { NULL, NULL }
    };
    
    if (!StartServiceCtrlDispatcher(serviceTable)) {
        DWORD error = GetLastError();
        if (error == ERROR_FAILED_SERVICE_CONTROLLER_CONNECT) {
            // Not running as service, run in console mode
            std::cout << "PS5 Camera Service - Console Mode" << std::endl;
            std::cout << "Use --console parameter or install as Windows Service" << std::endl;
            return 1;
        }
    }
    
    return 0;
}
