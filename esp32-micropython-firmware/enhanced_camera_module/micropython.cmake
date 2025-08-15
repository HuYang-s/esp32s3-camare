# Enhanced Camera Module CMake Configuration
# This file integrates the enhanced camera module with MicroPython build system

# Create an INTERFACE library for our enhanced camera module
add_library(usermod_camera_enhanced INTERFACE)

# Add our source files to the library
target_sources(usermod_camera_enhanced INTERFACE
    ${CMAKE_CURRENT_LIST_DIR}/modcamera_enhanced.c
)

# Add the current directory as an include directory
target_include_directories(usermod_camera_enhanced INTERFACE
    ${CMAKE_CURRENT_LIST_DIR}
)

# Include ESP32 camera component directories
if(EXISTS "${IDF_PATH}/components/esp32-camera")
    target_include_directories(usermod_camera_enhanced INTERFACE
        ${IDF_PATH}/components/esp32-camera/driver/include
        ${IDF_PATH}/components/esp32-camera/driver/private_include
        ${IDF_PATH}/components/esp32-camera/conversions/include
        ${IDF_PATH}/components/esp32-camera/conversions/private_include
        ${IDF_PATH}/components/esp32-camera/sensors/private_include
    )
endif()

# Try to find ESP-JPEG component in various locations
if(EXISTS "${CMAKE_CURRENT_LIST_DIR}/../../../ports/esp32/managed_components/espressif__esp_jpeg/include")
    target_include_directories(usermod_camera_enhanced INTERFACE
        ${CMAKE_CURRENT_LIST_DIR}/../../../ports/esp32/managed_components/espressif__esp_jpeg/include
    )
elseif(EXISTS "${IDF_PATH}/managed_components/espressif__esp_jpeg/include")
    target_include_directories(usermod_camera_enhanced INTERFACE
        ${IDF_PATH}/managed_components/espressif__esp_jpeg/include
    )
elseif(EXISTS "${CMAKE_CURRENT_LIST_DIR}/../../../components/esp_jpeg/include")
    target_include_directories(usermod_camera_enhanced INTERFACE
        ${CMAKE_CURRENT_LIST_DIR}/../../../components/esp_jpeg/include
    )
endif()

# Add compile definitions
target_compile_definitions(usermod_camera_enhanced INTERFACE
    MODULE_CAMERA_ENABLED=1
)

# Link our INTERFACE library to the usermod target
target_link_libraries(usermod INTERFACE usermod_camera_enhanced)