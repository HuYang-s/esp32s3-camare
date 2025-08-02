# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

This is an ESP32-S3 camera AI analysis project using ESP-IDF framework. The project connects to WiFi as a station, captures images with an OV3640 camera module, and sends them to NVIDIA AI API for analysis. Results are displayed via serial output.

## Development Commands

### Build and Flash
- `idf.py build` - Build the project
- `idf.py flash` - Flash firmware to ESP32-S3
- `idf.py monitor` - Monitor serial output
- `idf.py flash monitor` - Flash and immediately start monitoring
- `idf.py menuconfig` - Configure project settings

### Cleaning
- `idf.py clean` - Clean build artifacts
- `idf.py fullclean` - Complete clean including managed components

### Component Management
- Components are managed via ESP Component Manager (dependencies defined in `main/idf_component.yml`)
- External dependencies: `espressif/esp32-camera` for camera operations

## Architecture

### Core Components
- **Main Application** (`main/esp32s3_camera_web.c`): Contains WiFi STA setup, HTTP client, camera operations, and AI analysis
- **Camera Driver** (`components/camera_driver/`): Hardware abstraction layer for camera operations
- **Managed Components**: Uses ESP32-camera and ESP-JPEG components via ESP Component Manager

### Application Features
- **AI Image Analysis**: Automatic image capture and analysis using NVIDIA AI API
- **Serial Output**: Real-time status and AI analysis results via UART
- **WiFi Station Mode**: Connects to specified WiFi network for internet access
- **Sequential Processing**: Capture → Analyze → Wait → Repeat cycle
- **Optional Web Interface**: Basic web server for manual image capture
- **SPIFFS Storage**: Optional local image storage for debugging

### Task Architecture
- **Main Task**: WiFi connection, system initialization, and camera setup
- **Capture and Analysis Task**: Continuous cycle of image capture, AI analysis, and result output
- **Web Server Task** (optional): Serves basic web interface

### Camera Configuration
- Camera sensor: OV3640 
- Pin configuration defined in `camera_driver.c` for ESP32-S3
- JPEG output format with QVGA frame size (320x240)
- 20MHz XCLK frequency, JPEG quality level 15
- Single frame buffer in DRAM with grab-when-empty mode

### GPIO Pin Mapping (ESP32-S3 to OV3640)
```
XCLK:  GPIO 15    SIOD:  GPIO 4     SIOC:  GPIO 5
Y2:    GPIO 11    Y3:    GPIO 9     Y4:    GPIO 8
Y5:    GPIO 10    Y6:    GPIO 12    Y7:    GPIO 18
Y8:    GPIO 17    Y9:    GPIO 16
VSYNC: GPIO 6     HREF:  GPIO 7     PCLK:  GPIO 13
```

### WiFi Configuration
- Station mode connecting to "bed_room_2.4G"
- Password: "Hdk4560.0"
- Automatic reconnection on disconnection
- 30-second connection timeout

### AI Analysis Configuration
- **API**: NVIDIA Integrate API
- **Base URL**: https://integrate.api.nvidia.com
- **Model**: meta/llama-4-maverick-17b-128e-instruct
- **API Key**: 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
- **Analysis Interval**: 10 seconds between captures
- **Timeout**: 30 seconds per API request

## Key APIs

### Camera Driver Functions
- `camera_init()` - Initialize camera with hardware pin configuration
- `camera_capture(camera_fb_t** fb)` - Capture single frame
- `camera_return_fb(camera_fb_t* fb)` - Return frame buffer to system

### AI Analysis Functions
- `encode_image_to_base64(camera_fb_t *fb)` - Convert JPEG to Base64 encoding
- `analyze_image_with_ai(camera_fb_t *fb)` - Send image to AI API and process response
- `_http_event_handler()` - Handle HTTP client events and responses

### Core Application Flow
1. Initialize NVS and SPIFFS
2. Connect to WiFi network (with timeout)
3. Initialize camera hardware
4. Start continuous capture-analyze cycle:
   - Capture image from camera
   - Output "拍照完成" to serial
   - Send image to AI API for analysis
   - Display AI analysis results on serial
   - Wait 10 seconds
   - Repeat

## Hardware Requirements
- ESP32-S3 development board
- OV3640 camera module connected to specific GPIO pins as defined in camera_driver.c
- Stable WiFi internet connection for AI API access

## Memory and Storage
- **NVS Partition**: 24KB for non-volatile storage (WiFi credentials, etc.)
- **PHY Init**: 4KB for WiFi PHY initialization data  
- **Application**: 1MB for firmware code
- **SPIFFS**: 2.9MB for optional image storage
- **Total Flash**: ~4MB required (defined in `partitions.csv`)
- **RAM**: Increased stack size (8KB) for AI analysis task due to HTTP client and JSON processing

## Development Notes
- ESP-IDF version 5.4.2 required (minimum 4.1.0 supported)
- Additional dependencies: esp_http_client, json, mbedtls for AI API integration
- Component dependencies automatically managed via ESP Component Manager
- Camera sensor detection and initialization handled by esp32-camera component
- Base64 encoding uses mbedtls library
- JSON processing uses cJSON library
- Serial output includes both ESP_LOG messages and printf statements for user feedback
- Network connectivity required for AI analysis functionality