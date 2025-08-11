# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

This is an ESP32-S3 camera AI analysis project with L298N motor control using ESP-IDF framework. The project operates in WiFi AP+STA dual mode, captures images with camera modules (OV2640/OV3660/OV5640 etc.), sends them to AI APIs for analysis, and includes motor control capabilities for robotics applications. Results are displayed via web interface and serial output.

### Development Commands

### Build and Flash
- `idf.py build` - Build the project
- `idf.py flash` - Flash firmware to ESP32-S3
- `idf.py monitor` - Monitor serial output
- `idf.py flash monitor` - Flash and immediately start monitoring
- `idf.py menuconfig` - Configure project settings

### Cleaning
- `idf.py clean` - Clean build artifacts
- `idf.py fullclean` - Complete clean including managed components

### Debugging and Development
- `idf.py monitor` - Serial monitor with automatic log filtering and formatting
- `idf.py menuconfig` - Interactive configuration menu (includes camera settings, WiFi config, etc.)
- `idf.py size` - Analyze memory usage of the compiled firmware
- `idf.py app-flash` - Flash only the application (faster than full flash during development)

### Component Management
- Components are managed via ESP Component Manager (dependencies defined in `main/idf_component.yml`)
- External dependencies: `espressif/esp32-camera` with automatic esp_jpeg dependency resolution
- Lock file (`dependencies.lock`) tracks exact component versions for reproducible builds
- `idf.py reconfigure` - Refresh component dependencies after changes to `idf_component.yml`

## Architecture

### Core Components
- **Main Application** (`main/esp32s3_camera_web.c`): System initialization, task management, and orchestration
- **Camera Driver** (`components/camera_driver/`): Hardware abstraction layer for camera operations
- **AI Service** (`components/ai_service/`): AI analysis service for image processing with API integration and text parsing
- **WiFi Manager** (`components/wifi_manager/`): WiFi AP+STA dual mode management
- **Web Server** (`components/web_server/`): HTTP server for web interface and API endpoints
- **Storage Manager** (`components/storage_manager/`): SPIFFS-based image storage and history management
- **Time Service** (`components/time_service/`): SNTP time synchronization and Beijing timezone handling
- **Motor Driver** (`components/motor_driver/`): L298N motor control with direction and speed management
- **Managed Components**: ESP32-camera and ESP-JPEG components via ESP Component Manager

### Application Features
- **AI Assistant**: Intelligent assistant with self-awareness that treats camera input as its visual perception
- **Smart Navigation**: Advanced obstacle avoidance with intelligent path planning and route optimization
- **Active Exploration**: AI actively searches for hidden, partially obscured, or edge-of-vision content
- **Obstacle Avoidance**: Intelligent detection and avoidance of walls, barriers, and dangerous areas
- **Curiosity-Driven Search**: AI turns to investigate areas that are partially visible or mysteriously hidden
- **Self-Directed Navigation**: AI model uses tool calls to autonomously decide movement direction based on environmental analysis
- **Conscious Decision Making**: AI assistant evaluates environment safety, interesting targets, and unexplored areas
- **Spatial Intelligence**: AI understands room layouts, corners, and hidden spaces requiring investigation
- **Motor Control**: L298N dual motor driver for robotics movement controlled by intelligent AI decisions
- **Serial Output**: Real-time status, AI analysis results, and motor control feedback via UART
- **WiFi Dual Mode**: Operates in both AP and STA modes simultaneously for flexible connectivity
- **Continuous Processing**: Observe → Analyze → Plan → Navigate → Explore cycle with intelligent decision-making
- **Web Interface**: Full-featured web server displaying image history with AI analysis results
- **Real-time Clock**: Network-synchronized Beijing time display on web interface
- **Smart Auto-refresh**: Configurable automatic page refresh (6s visible, 15s hidden)
- **Network Status Display**: Real-time AP and STA connection monitoring on web interface
- **SNTP Time Sync**: Automatic network time synchronization with Chinese NTP servers
- **SPIFFS Storage**: Local image storage with automatic history management (3 image limit)
- **Image History Management**: Automatic cleanup of old images when limit exceeded
- **Resilient Error Handling**: Network failure recovery with backoff strategies for AI APIs
- **Motor Test Sequences**: Automated movement testing on system startup
- **Mode Switching**: Automatic switching between normal AI analysis and intelligent exploration modes every 10 capture cycles

### Task Architecture
- **Main Task**: System initialization, WiFi AP+STA setup, camera and motor driver initialization
- **Capture and Analysis Task**: Continuous cycle of image capture, AI analysis, and result output (12KB stack)
- **Motor Test Task**: Automated motor test sequence on startup (4KB stack)
- **Web Server Task**: Serves web interface with image history and real-time updates
- **Mutex Management**: Camera access controlled via mutex for thread safety between tasks

### Camera Configuration
- Camera sensor: Auto-detected (supports multiple sensors including OV2640, OV3660, OV5640, etc.)
- Pin configuration defined in `camera_driver.c` for ESP32-S3
- JPEG output format with QVGA frame size (320x240)
- 20MHz XCLK frequency, JPEG quality level 12
- Single frame buffer in DRAM with grab-when-empty mode

### GPIO Pin Mapping (ESP32-S3 to Camera Module)
```
XCLK:  GPIO 15    SIOD:  GPIO 4     SIOC:  GPIO 5
Y2:    GPIO 11    Y3:    GPIO 9     Y4:    GPIO 8
Y5:    GPIO 10    Y6:    GPIO 12    Y7:    GPIO 18
Y8:    GPIO 17    Y9:    GPIO 16
VSYNC: GPIO 6     HREF:  GPIO 7     PCLK:  GPIO 13
```

### L298N Motor Control GPIO Mapping
```
IN1:   GPIO 3     (Left motor forward)
IN2:   GPIO 1     (Left motor backward) 
IN3:   GPIO 2     (Right motor forward)
IN4:   GPIO 42    (Right motor backward)
```

### Partition Table (partitions.csv)
- **NVS**: 24KB for non-volatile storage (WiFi credentials, etc.)
- **PHY Init**: 4KB for WiFi PHY initialization data
- **Factory App**: 3MB for firmware code (0x300000 bytes)  
- **SPIFFS**: 2.9MB for image storage (0x2F0000 bytes)
- **Total Flash**: ~6MB required

### WiFi Configuration (AP+STA Dual Mode)
- **Dual Mode Operation**: ESP32-S3 operates in both Access Point (AP) and Station (STA) modes simultaneously
- **STA Mode**: Connects to existing WiFi network "bed_room_2.4G" for internet access and AI API communication
- **AP Mode**: Creates WiFi hotspot "ESP32-S3-Camera" (password: "12345678") for direct device access
- **AP Configuration**: Channel 1, maximum 4 concurrent connections, WPA2-PSK authentication
- **Automatic Reconnection**: STA mode automatically reconnects on disconnection
- **30-second Connection Timeout**: STA mode connection timeout for initial WiFi setup
- **Network Status API**: Real-time monitoring of both AP and STA connection states via `/api/network`

### AI Analysis Configuration
- **AI Assistant**: Mistral API integration using mistral-small-latest model with self-aware assistant personality
- **Intelligence Modes**: 
  - **Normal Mode**: AI assistant observes and describes environment through its "camera eyes"
  - **Smart Explorer Mode**: Advanced AI navigation with obstacle avoidance and active hidden area searching
- **Navigation Intelligence**:
  - **Obstacle Detection**: Smart recognition of walls, barriers, and dangerous areas
  - **Path Planning**: Intelligent route selection to bypass obstacles safely
  - **Active Search**: AI proactively investigates partially visible or edge-of-vision content
  - **Spatial Mapping**: AI builds mental map of explored vs unexplored areas
- **Exploration Strategies**:
  - **Safety First**: Always prioritize obstacle avoidance and safe navigation
  - **Search Unknown**: Actively seek out hidden corners, obscured objects, and mysterious areas  
  - **Target Approach**: Move closer to interesting objects for detailed observation
  - **Complete Coverage**: Ensure thorough exploration of all accessible areas
  - **Novelty Priority**: Focus on previously unseen or particularly intriguing discoveries
- **Model Parameters**: max_tokens=128, temperature=0.7, top_p=0.7, frequency_penalty=0.5, n=1
- **Decision Framework**: AI weighs safety, curiosity, exploration completeness, and discovery potential
- **Tool Call Integration**: Autonomous motor control through function calling for intelligent navigation
- **Error Recovery**: Exponential backoff for network failures, automatic retry logic with failure counters
- **Processing Mode**: Continuous analysis with configurable retry limits and adaptive delays
- **Memory Management**: Dynamic allocation with proper cleanup and memory checks for Base64 encoding
- **Timeout Handling**: Configurable timeouts for AI API requests (15 seconds)
- **Base64 Encoding**: Large memory allocations (2x image size) for API transmission
- **Text Processing**: Integrated text parser for AI response handling and command extraction
- **Chinese Language**: All prompts and responses in Chinese language with natural assistant personality

### Motor Control Configuration
- **Motor Driver**: L298N dual H-bridge motor driver
- **Movement Types**: Forward, backward, left turn, right turn, stop
- **Speed Control**: Variable speed control (0-100% range)
- **Test Sequence**: Automated startup test (forward 2s → stop 1s → backward 2s → stop 1s → left 1.5s → stop 1s → right 1.5s → stop)
- **Safety Features**: Automatic motor stop and GPIO isolation on errors

## Key APIs

### Camera Driver Functions
- `camera_init()` - Initialize camera with hardware pin configuration
- `camera_capture(camera_fb_t** fb)` - Capture single frame
- `camera_return_fb(camera_fb_t* fb)` - Return frame buffer to system

### AI Service Functions  
- `ai_service_analyze_image(camera_fb_t *fb, const char* filename)` - Send image to AI API and process response
- `ai_service_auto_drive_analyze(camera_fb_t *fb, const char* filename)` - Auto-driving mode AI analysis with motor control integration
- `ai_service_get_socket_failure_count()` - Get current network failure count for error handling

### Motor Driver Functions
- `motor_driver_init()` - Initialize L298N motor driver GPIOs
- `motor_forward(int speed)` - Move forward with specified speed (0-100)
- `motor_backward(int speed)` - Move backward with specified speed
- `motor_left(int speed)` - Turn left with specified speed
- `motor_right(int speed)` - Turn right with specified speed
- `motor_stop_all()` - Stop all motor movement
- `motor_test_sequence()` - Execute automated motor test sequence

### WiFi Manager Functions
- `wifi_manager_init_ap_sta()` - Initialize WiFi in AP+STA dual mode
- `wifi_manager_wait_for_connect()` - Wait for STA connection with timeout

### Storage Manager Functions
- `storage_manager_init()` - Initialize SPIFFS storage system
- `storage_manager_save_image()` - Save camera frame to SPIFFS with timestamp
- `storage_manager_update_ai_result()` - Update stored image with AI analysis results
- `storage_manager_get_image_history()` - Retrieve image history with thread safety

### Time Service Functions
- `time_service_init()` - Initialize SNTP client with Beijing timezone

### Web Server Functions
- `web_server_start()` - Start HTTP server with API endpoints and static content

### Core Application Flow
1. Initialize NVS and SPIFFS storage systems
2. Initialize WiFi in AP+STA dual mode (connects to existing network + creates hotspot)  
3. Initialize SNTP time synchronization with Beijing timezone
4. Initialize camera hardware with auto-detection (AI's "eyes")
5. Initialize L298N motor driver with GPIO configuration (AI's "body")
6. Start web server for image history and API endpoints
7. Start motor test task (automated movement testing)
8. Start intelligent AI exploration cycle:
   - 👁️ **Visual Analysis**: AI observes environment through camera with detailed spatial understanding
   - 🧠 **Intelligent Decision**: AI analyzes obstacles, safe paths, and interesting unexplored areas
   - 🚧 **Obstacle Avoidance**: Smart detection and bypassing of walls, barriers, and dangerous zones
   - 🔍 **Active Search**: AI proactively investigates hidden corners, partially visible objects, and edge areas
   - 🎯 **Target Selection**: Choose most promising exploration direction based on safety and curiosity
   - 🤖 **Motor Control**: Execute movement decision via tool calls with optimal duration and speed
   - 📊 **Result Logging**: Record AI reasoning, decisions, and exploration outcomes
   - 🔄 **Continuous Learning**: Build spatial awareness and adapt exploration strategies
   - ⚡ **Error Recovery**: Handle failures with intelligent backoff and alternative path planning

## Hardware Requirements
- ESP32-S3 development board with sufficient GPIO pins
- Compatible camera module (OV2640, OV3660, OV5640, etc.) connected per GPIO pin mapping
- L298N motor driver module with dual motors for robotics applications
- Stable 5V power supply (2A+ recommended) for L298N motor driver
- Stable WiFi internet connection for AI API access (STA mode)
- Mobile device or computer for direct connection (AP mode)

## Memory and Storage
- **NVS Partition**: 24KB for non-volatile storage (WiFi credentials, etc.)
- **PHY Init**: 4KB for WiFi PHY initialization data  
- **Factory App**: 3MB for firmware code (0x300000 bytes)
- **SPIFFS**: 2.9MB for image storage (0x2F0000 bytes)
- **Total Flash**: ~6MB required (defined in `partitions.csv`)
- **RAM**: Increased stack size (12KB) for AI analysis task due to HTTP client and JSON processing
- **Image Storage**: Maximum 3 images in SPIFFS with automatic cleanup

## Development Notes
- ESP-IDF version 5.4.2 required (minimum 4.1.0 supported)
- Modular component-based architecture with clear separation of concerns
- Component dependencies automatically managed via ESP Component Manager
- Camera sensor detection and initialization handled by esp32-camera component
- Serial output includes both ESP_LOG messages and printf statements for user feedback
- Network connectivity required for AI analysis functionality
- Motor control designed with GPIO conflict avoidance for camera pins

## Component Architecture Details
- **Main application** acts as orchestrator, initializing and coordinating all components
- Each component is self-contained with its own header files and CMakeLists.txt
- Components communicate through well-defined APIs and avoid direct dependencies
- **Thread Synchronization**: Camera mutex, WiFi semaphore, and history mutex for safe resource sharing
- **Task Priorities**: Capture task (priority 5), Motor test task (priority 3), Web server managed by ESP-IDF
- **Component Dependencies**: Clear dependency graph with ai_service depending on multiple components
- Error handling with failure counters and exponential backoff strategies

### Security Considerations
- **CRITICAL**: API keys and WiFi credentials are hardcoded in source files
- Consider using NVS storage or environment variables for sensitive data
- WiFi credentials: components/wifi_manager/wifi_manager.c:17-18 (WIFI_SSID, WIFI_PASS)
- AI API key: components/ai_service/ai_service.c:20 (AI_API_KEY - Mistral API)
- AP password: main/esp32s3_camera_web.c:20 (AP_PASS)

### Troubleshooting
- **Camera initialization fails**: Check GPIO connections match camera_driver.c pin definitions
- **Motor not responding**: Verify L298N power supply (5V, 2A+) and GPIO connections (GPIO 3,1,2,42)
- **WiFi connection issues**: Check WiFi credentials in wifi_manager component
- **AI analysis failures**: Monitor failure counters and ensure internet connectivity
- **Memory crashes**: Use `idf.py size` to monitor memory usage, increase stack sizes if needed
- **Serial monitoring**: Use `idf.py monitor` for real-time debugging output
- **GPIO conflicts**: Camera pins (4,5,6,7,8,9,10,11,12,13,15,16,17,18) reserved, motor uses (1,2,3,42)

### Motor Control Troubleshooting
- **Motors don't move**: Check 5V power supply to L298N module
- **Wrong direction**: Swap IN1/IN2 or IN3/IN4 connections for affected motor
- **System resets during motor operation**: Insufficient power supply or short circuit
- **Motor test sequence fails**: Check serial output for GPIO initialization errors