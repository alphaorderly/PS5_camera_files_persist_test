cmake_minimum_required(VERSION 3.16)
project(PS5CameraService)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Windows specific settings
if(WIN32)
    add_definitions(-DWIN32 -D_WINDOWS -DUNICODE -D_UNICODE)
    
    # Include directories
    include_directories(source)
    
    # PS5 Camera Service executable
    add_executable(PS5CameraService
        source/PS5CameraService.cpp
        source/OrbisEyeCam.cpp
        source/OrbisEyeCam.h
        source/ServiceGUIDs.h
        source/pch.h
    )
    
    # Original loader executable (for testing)
    add_executable(PS5CameraLoader
        source/main.cpp
        source/OrbisEyeCam.cpp
        source/OrbisEyeCam.h
        source/pch.h
    )
    
    # Link Windows libraries
    target_link_libraries(PS5CameraService
        kernel32
        user32
        setupapi
        winusb
        advapi32
    )
    
    target_link_libraries(PS5CameraLoader
        kernel32
        user32
        setupapi
        winusb
    )
    
    # Set subsystem
    set_target_properties(PS5CameraService PROPERTIES
        WIN32_EXECUTABLE TRUE
        LINK_FLAGS "/SUBSYSTEM:CONSOLE"
    )
    
    set_target_properties(PS5CameraLoader PROPERTIES
        WIN32_EXECUTABLE TRUE
        LINK_FLAGS "/SUBSYSTEM:CONSOLE"
    )
    
    # Copy firmware file to output directory
    add_custom_command(TARGET PS5CameraService POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_SOURCE_DIR}/firmware_discord_and_gamma_fix.bin"
        "$<TARGET_FILE_DIR:PS5CameraService>/firmware_discord_and_gamma_fix.bin"
    )
    
endif()

# Installation
if(WIN32)
    install(TARGETS PS5CameraService PS5CameraLoader
        RUNTIME DESTINATION bin
    )
    
    install(FILES firmware_discord_and_gamma_fix.bin
        DESTINATION bin
    )
    
    install(FILES 
        install.bat
        uninstall.bat
        README.md
        DESTINATION .
    )
endif()
