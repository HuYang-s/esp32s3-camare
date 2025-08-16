# ESP32-S3摄像头AI分析系统 - 架构图和数据流图

## 概述

本文档提供了ESP32-S3摄像头AI分析系统的详细架构图和数据流图，帮助开发者理解系统的整体设计和各组件之间的交互关系。

## 1. 系统整体架构图

### 1.1 分层架构图

```mermaid
graph TB
    subgraph "用户界面层"
        Web[Web界面]
        API[REST API]
        WS[WebSocket]
    end
    
    subgraph "应用服务层"
        Main[主程序<br/>esp32s3_camera_web.c]
        WebServer[Web服务器<br/>web_server.c]
        AIService[AI服务<br/>ai_service.c]
        LocalAI[本地AI服务<br/>local_ai_service.c]
    end
    
    subgraph "业务逻辑层"
        Motor[电机驱动<br/>motor_driver.c]
        Camera[摄像头驱动<br/>camera_driver.c]
        Storage[存储管理<br/>storage_manager.c]
        WiFi[WiFi管理<br/>wifi_manager.c]
        Navigation[导航服务<br/>navigation_service.c]
        Time[时间服务<br/>time_service.c]
    end
    
    subgraph "硬件抽象层"
        GPIO[GPIO控制]
        SPI[SPI接口]
        I2C[I2C接口]
        UART[UART接口]
        Flash[Flash存储]
        WiFiHW[WiFi硬件]
    end
    
    subgraph "硬件层"
        ESP32[ESP32-S3芯片]
        CameraMod[摄像头模块]
        L298N[L298N电机驱动]
        Motors[直流电机]
        Antenna[WiFi天线]
    end
    
    %% 连接关系
    Web --> WebServer
    API --> WebServer
    WS --> WebServer
    
    Main --> AIService
    Main --> LocalAI
    Main --> Motor
    Main --> Camera
    Main --> Storage
    
    WebServer --> AIService
    WebServer --> Motor
    WebServer --> Storage
    
    AIService --> LocalAI
    AIService --> WiFi
    
    Motor --> GPIO
    Camera --> SPI
    Camera --> I2C
    Storage --> Flash
    WiFi --> WiFiHW
    
    GPIO --> ESP32
    SPI --> ESP32
    I2C --> ESP32
    Flash --> ESP32
    WiFiHW --> ESP32
    
    ESP32 --> CameraMod
    ESP32 --> L298N
    L298N --> Motors
    ESP32 --> Antenna
```

### 1.2 组件依赖关系图

```
┌─────────────────────────────────────────────────────────────────┐
│                        ESP32-S3 主控制器                         │
├─────────────────────────────────────────────────────────────────┤
│  main/esp32s3_camera_web.c (主程序入口)                          │
│  ├── 系统初始化                                                   │
│  ├── FreeRTOS任务管理                                            │
│  └── 核心业务循环                                                 │
├─────────────────┬───────────────┬───────────────┬─────────────────┤
│   Web服务器     │    AI服务     │   电机驱动    │   摄像头驱动    │
│  web_server.c   │ ai_service.c  │motor_driver.c │camera_driver.c  │
│  ├── HTTP服务   │ ├── 云端AI    │ ├── L298N控制 │ ├── 图像采集    │
│  ├── API端点    │ ├── 本地AI    │ ├── 精确控制  │ ├── 参数配置    │
│  ├── WebSocket  │ ├── Tool Call │ ├── 差速驱动  │ └── 格式转换    │
│  └── 静态文件   │ └── 决策分析  │ └── 角度控制  │                 │
├─────────────────┼───────────────┼───────────────┼─────────────────┤
│   WiFi管理      │   存储管理    │   导航服务    │   时间服务      │
│ wifi_manager.c  │storage_mgr.c  │navigation.c   │time_service.c   │
│ ├── AP模式      │ ├── SPIFFS    │ ├── 路径规划  │ ├── NTP同步     │
│ ├── STA模式     │ ├── 图像存储  │ ├── 区域搜索  │ ├── 时间戳      │
│ ├── 双模式      │ ├── 文件管理  │ └── 避障逻辑  │ └── 定时任务    │
│ └── 网络监控    │ └── 元数据    │               │                 │
├─────────────────┴───────────────┴───────────────┴─────────────────┤
│                        硬件抽象层                                 │
├─────────────┬─────────────┬─────────────┬─────────────┬─────────────┤
│    GPIO     │     SPI     │     I2C     │    UART     │   Flash     │
│   控制      │    接口     │    接口     │    接口     │   存储      │
└─────────────┴─────────────┴─────────────┴─────────────┴─────────────┘
```

## 2. 数据流图

### 2.1 主要数据流程图

```mermaid
flowchart LR
    subgraph "数据输入"
        Camera[摄像头<br/>JPEG图像]
        WebUI[Web界面<br/>用户指令]
        Network[网络<br/>AI响应]
    end
    
    subgraph "数据处理"
        Capture[图像采集]
        Storage[存储保存]
        LocalAI[本地AI检测]
        CloudAI[云端AI分析]
        Decision[决策引擎]
    end
    
    subgraph "数据输出"
        Motor[电机控制]
        WebResp[Web响应]
        Log[日志记录]
        Files[文件存储]
    end
    
    %% 数据流向
    Camera --> Capture
    Capture --> Storage
    Storage --> Files
    
    Capture --> LocalAI
    LocalAI --> Decision
    
    Capture --> CloudAI
    Network --> CloudAI
    CloudAI --> Decision
    
    Decision --> Motor
    Decision --> WebResp
    Decision --> Log
    
    WebUI --> Decision
    Decision --> WebUI
```

### 2.2 AI分析数据流

```
摄像头图像采集流程：
┌─────────────┐    ┌─────────────┐    ┌─────────────┐
│   摄像头    │───→│  图像缓冲区  │───→│  JPEG编码   │
│  (硬件)     │    │camera_fb_t  │    │   数据      │
└─────────────┘    └─────────────┘    └─────────────┘
                                              │
                                              ▼
┌─────────────┐    ┌─────────────┐    ┌─────────────┐
│  文件存储   │◄───│  存储管理   │◄───│  图像数据   │
│  (SPIFFS)   │    │   模块      │    │   处理      │
└─────────────┘    └─────────────┘    └─────────────┘
                                              │
                                              ▼
                            ┌─────────────────────────────┐
                            │        AI分析分支           │
                            └─────────────────────────────┘
                                    │                │
                                    ▼                ▼
                        ┌─────────────┐    ┌─────────────┐
                        │   本地AI    │    │   云端AI    │
                        │    检测     │    │    分析     │
                        └─────────────┘    └─────────────┘
                                    │                │
                                    ▼                ▼
                        ┌─────────────┐    ┌─────────────┐
                        │  特征检测   │    │ Base64编码  │
                        │  (颜色/形状) │    │  HTTP请求   │
                        └─────────────┘    └─────────────┘
                                    │                │
                                    ▼                ▼
                        ┌─────────────┐    ┌─────────────┐
                        │  检测结果   │    │  JSON解析   │
                        │detection_t  │    │ Tool Call   │
                        └─────────────┘    └─────────────┘
                                    │                │
                                    └────────┬───────┘
                                             ▼
                                  ┌─────────────────┐
                                  │    决策融合     │
                                  │   (优先本地)    │
                                  └─────────────────┘
                                             │
                                             ▼
                                  ┌─────────────────┐
                                  │    执行动作     │
                                  │  (电机控制)     │
                                  └─────────────────┘
```

### 2.3 电机控制数据流

```
电机控制指令流程：
┌─────────────┐    ┌─────────────┐    ┌─────────────┐
│  Web界面    │───→│  HTTP API   │───→│  JSON解析   │
│   用户      │    │   请求      │    │   参数      │
└─────────────┘    └─────────────┘    └─────────────┘
                                              │
┌─────────────┐    ┌─────────────┐    ┌─────────────┐
│  AI决策     │───→│ Tool Call   │───→│  动作指令   │
│   引擎      │    │   执行      │    │   解析      │
└─────────────┘    └─────────────┘    └─────────────┘
                                              │
                                              ▼
                            ┌─────────────────────────────┐
                            │       电机控制分发          │
                            └─────────────────────────────┘
                                    │        │        │
                                    ▼        ▼        ▼
                        ┌─────────────┐ ┌─────────────┐ ┌─────────────┐
                        │  基础控制   │ │  角度控制   │ │  差速控制   │
                        │forward/back │ │turn_angle   │ │differential │
                        └─────────────┘ └─────────────┘ └─────────────┘
                                    │        │        │
                                    └────────┼────────┘
                                             ▼
                                  ┌─────────────────┐
                                  │   GPIO控制      │
                                  │ (IN1/IN2/IN3/IN4)│
                                  └─────────────────┘
                                             │
                                             ▼
                                  ┌─────────────────┐
                                  │   L298N驱动     │
                                  │   (硬件)        │
                                  └─────────────────┘
                                             │
                                             ▼
                                  ┌─────────────────┐
                                  │   直流电机      │
                                  │  (左轮/右轮)    │
                                  └─────────────────┘
```

## 3. 任务调度图

### 3.1 FreeRTOS任务架构

```mermaid
gantt
    title ESP32-S3 FreeRTOS任务调度
    dateFormat X
    axisFormat %s
    
    section 高优先级任务(5)
    图像采集分析任务    :active, task1, 0, 3600
    Web服务器任务      :active, task2, 0, 3600
    
    section 中优先级任务(3-4)
    AI处理任务         :active, task3, 0, 3600
    WiFi管理任务       :active, task4, 0, 3600
    
    section 低优先级任务(1-2)
    电机测试任务       :task5, 5, 10
    存储清理任务       :task6, 0, 3600
    系统监控任务       :active, task7, 0, 3600
```

### 3.2 任务间通信图

```
任务间通信机制：
┌─────────────────┐    ┌─────────────────┐    ┌─────────────────┐
│  主分析任务     │    │   Web服务器     │    │   AI处理任务    │
│capture_analyze  │    │   web_server    │    │  ai_processing  │
│   (优先级5)     │    │   (优先级5)     │    │   (优先级4)     │
└─────────────────┘    └─────────────────┘    └─────────────────┘
         │                       │                       │
         ▼                       ▼                       ▼
┌─────────────────────────────────────────────────────────────────┐
│                      共享资源和通信机制                          │
├─────────────────┬─────────────────┬─────────────────┬─────────────────┤
│   互斥锁        │    队列         │   信号量        │   共享内存      │
│camera_mutex     │  command_queue  │  sync_sem       │  status_data    │
│motor_mutex      │  result_queue   │  ready_sem      │  config_data    │
│storage_mutex    │  event_queue    │  complete_sem   │  metrics_data   │
└─────────────────┴─────────────────┴─────────────────┴─────────────────┘
         ▲                       ▲                       ▲
         │                       │                       │
┌─────────────────┐    ┌─────────────────┐    ┌─────────────────┐
│  电机控制任务   │    │  存储管理任务   │    │  系统监控任务   │
│ motor_control   │    │storage_manager  │    │system_monitor   │
│   (优先级3)     │    │   (优先级2)     │    │   (优先级1)     │
└─────────────────┘    └─────────────────┘    └─────────────────┘
```

## 4. 内存布局图

### 4.1 ESP32-S3内存分配

```
ESP32-S3内存布局 (总计8MB Flash + 512KB SRAM):
┌─────────────────────────────────────────────────────────────────┐
│                        Flash存储 (8MB)                          │
├─────────────────┬─────────────────┬─────────────────┬─────────────────┤
│   Bootloader    │   应用程序      │    SPIFFS       │    保留区域     │
│    (64KB)       │   (1.6MB)       │   (320KB)       │   (剩余空间)    │
│  0x1000-0x10000 │0x10000-0x1B0000 │0x1B0000-0x200000│  0x200000-...   │
└─────────────────┴─────────────────┴─────────────────┴─────────────────┘

┌─────────────────────────────────────────────────────────────────┐
│                        SRAM内存 (512KB)                         │
├─────────────────┬─────────────────┬─────────────────┬─────────────────┤
│   代码缓存      │    数据段       │     堆内存      │    栈内存       │
│   (I-Cache)     │   (Data/BSS)    │    (Heap)       │   (Stack)       │
│    ~128KB       │     ~64KB       │    ~256KB       │    ~64KB        │
└─────────────────┴─────────────────┴─────────────────┴─────────────────┘

任务栈分配:
┌─────────────────────────────────────────────────────────────────┐
│                        任务栈空间                               │
├─────────────────┬─────────────────┬─────────────────┬─────────────────┤
│  主分析任务     │   Web服务器     │   AI处理任务    │   其他任务      │
│   12KB Stack    │   16KB Stack    │    8KB Stack    │   4KB Stack     │
│capture_analyze  │   web_server    │  ai_processing  │   各种服务      │
└─────────────────┴─────────────────┴─────────────────┴─────────────────┘
```

### 4.2 缓冲区管理

```
图像处理缓冲区管理:
┌─────────────────────────────────────────────────────────────────┐
│                      图像数据流                                 │
└─────────────────────────────────────────────────────────────────┘
                                  │
                                  ▼
                        ┌─────────────────┐
                        │  摄像头帧缓冲区 │ ◄─── 硬件DMA
                        │   camera_fb_t   │
                        │   (~60KB JPEG)  │
                        └─────────────────┘
                                  │
                                  ▼
                        ┌─────────────────┐
                        │   临时处理区    │ ◄─── malloc分配
                        │   temp_buffer   │
                        │   (动态大小)    │
                        └─────────────────┘
                                  │
                                  ▼
                        ┌─────────────────┐
                        │  Base64编码区   │ ◄─── malloc分配
                        │  base64_buffer  │
                        │  (~80KB编码后)  │
                        └─────────────────┘
                                  │
                                  ▼
                        ┌─────────────────┐
                        │  HTTP传输缓冲   │ ◄─── 静态分配
                        │ http_buffer[8K] │
                        │  (发送/接收)    │
                        └─────────────────┘

网络通信缓冲区:
┌─────────────────┬─────────────────┬─────────────────┐
│  HTTP接收缓冲   │  HTTP发送缓冲   │  WebSocket缓冲  │
│   8KB静态       │   8KB静态       │   4KB静态       │
│  MAX_HTTP_RECV  │  MAX_HTTP_SEND  │   WS_BUFFER     │
└─────────────────┴─────────────────┴─────────────────┘
```

## 5. 网络架构图

### 5.1 网络拓扑图

```mermaid
graph TB
    subgraph "外部网络"
        Internet[互联网]
        Router[路由器<br/>192.168.1.1]
        Cloud[云端AI服务<br/>Mistral API]
    end
    
    subgraph "ESP32-S3设备"
        ESP32[ESP32-S3<br/>双WiFi模式]
        WebServer[Web服务器<br/>端口80]
        WebSocket[WebSocket<br/>实时通信]
    end
    
    subgraph "客户端设备"
        Phone[手机浏览器]
        PC[电脑浏览器]
        API_Client[API客户端]
    end
    
    %% 网络连接
    Internet --> Router
    Router --> ESP32
    ESP32 --> Cloud
    
    ESP32 --> WebServer
    WebServer --> WebSocket
    
    Phone --> WebServer
    PC --> WebServer
    API_Client --> WebServer
    
    Phone --> WebSocket
    PC --> WebSocket
```

### 5.2 协议栈图

```
ESP32-S3网络协议栈:
┌─────────────────────────────────────────────────────────────────┐
│                        应用层                                   │
├─────────────────┬─────────────────┬─────────────────┬─────────────────┤
│      HTTP       │    WebSocket    │      JSON       │      API        │
│   Web服务器     │    实时通信     │    数据格式     │    接口层       │
└─────────────────┴─────────────────┴─────────────────┴─────────────────┘
┌─────────────────────────────────────────────────────────────────┐
│                        传输层                                   │
├─────────────────────────────────┬─────────────────────────────────┤
│               TCP               │               UDP               │
│        (可靠传输)               │           (快速传输)           │
└─────────────────────────────────┴─────────────────────────────────┘
┌─────────────────────────────────────────────────────────────────┐
│                        网络层                                   │
├─────────────────────────────────────────────────────────────────┤
│                          IPv4                                   │
│                    (网络路由)                                   │
└─────────────────────────────────────────────────────────────────┘
┌─────────────────────────────────────────────────────────────────┐
│                       数据链路层                                │
├─────────────────────────────────────────────────────────────────┤
│                        WiFi 802.11                              │
│                   (无线局域网)                                  │
└─────────────────────────────────────────────────────────────────┘
┌─────────────────────────────────────────────────────────────────┐
│                        物理层                                   │
├─────────────────────────────────────────────────────────────────┤
│                      2.4GHz射频                                 │
│                    (无线信号)                                   │
└─────────────────────────────────────────────────────────────────┘
```

## 6. 状态机图

### 6.1 AI任务状态机

```mermaid
stateDiagram-v2
    [*] --> IDLE
    
    IDLE --> SEARCHING : start_task()
    SEARCHING --> COMPLETED : target_found
    SEARCHING --> FAILED_TIMEOUT : timeout
    SEARCHING --> FAILED_UNABLE : error
    SEARCHING --> IDLE : stop_task()
    
    COMPLETED --> IDLE : reset()
    FAILED_TIMEOUT --> IDLE : reset()
    FAILED_UNABLE --> IDLE : reset()
    
    SEARCHING --> SCANNING : navigation_mode
    SCANNING --> MOVING : need_reposition
    MOVING --> SCANNING : position_reached
    SCANNING --> COMPLETED : target_found
    SCANNING --> NAVIGATING : explore_new_area
    NAVIGATING --> SCANNING : area_reached
```

### 6.2 电机控制状态机

```mermaid
stateDiagram-v2
    [*] --> STOPPED
    
    STOPPED --> MOVING_FORWARD : forward()
    STOPPED --> MOVING_BACKWARD : backward()
    STOPPED --> TURNING_LEFT : left()
    STOPPED --> TURNING_RIGHT : right()
    STOPPED --> ANGLE_TURNING : turn_angle()
    STOPPED --> DIFFERENTIAL : differential_drive()
    
    MOVING_FORWARD --> STOPPED : stop() / timeout
    MOVING_BACKWARD --> STOPPED : stop() / timeout
    TURNING_LEFT --> STOPPED : stop() / timeout
    TURNING_RIGHT --> STOPPED : stop() / timeout
    ANGLE_TURNING --> STOPPED : angle_complete
    DIFFERENTIAL --> STOPPED : duration_complete
    
    MOVING_FORWARD --> TURNING_LEFT : direction_change
    MOVING_FORWARD --> TURNING_RIGHT : direction_change
    TURNING_LEFT --> MOVING_FORWARD : direction_change
    TURNING_RIGHT --> MOVING_FORWARD : direction_change
```

## 7. 时序图

### 7.1 AI搜索任务时序图

```mermaid
sequenceDiagram
    participant User as 用户
    participant Web as Web界面
    participant API as API服务
    participant AI as AI服务
    participant Motor as 电机控制
    participant Camera as 摄像头
    
    User->>Web: 点击"开始搜索"
    Web->>API: POST /api/ai-task {"action":"start","target":"laptop"}
    API->>AI: local_ai_start_task("laptop", 30)
    AI->>AI: 初始化搜索任务
    API-->>Web: {"success": true, "message": "任务已启动"}
    Web-->>User: 显示"搜索中..."
    
    loop 搜索循环
        AI->>Camera: 请求图像
        Camera-->>AI: 返回camera_fb_t
        AI->>AI: local_ai_detect_objects()
        
        alt 检测到目标
            AI->>API: 更新任务状态(COMPLETED)
            API-->>Web: 状态更新
            Web-->>User: 显示"找到目标!"
        else 未检测到目标
            AI->>Motor: 调整位置
            Motor->>Motor: 执行移动
            AI->>API: 更新进度
            API-->>Web: 进度更新
        end
    end
    
    alt 超时
        AI->>API: 更新任务状态(FAILED_TIMEOUT)
        API-->>Web: 超时通知
        Web-->>User: 显示"搜索超时"
    end
```

### 7.2 电机控制时序图

```mermaid
sequenceDiagram
    participant User as 用户
    participant Web as Web界面
    participant API as API服务
    participant Motor as 电机驱动
    participant GPIO as GPIO控制
    participant Hardware as L298N硬件
    
    User->>Web: 点击"前进"按钮
    Web->>API: POST /api/motor {"action":"forward","duration":2000}
    API->>Motor: motor_forward()
    
    Motor->>GPIO: gpio_set_level(IN1, 1)
    Motor->>GPIO: gpio_set_level(IN2, 0)
    Motor->>GPIO: gpio_set_level(IN3, 1)
    Motor->>GPIO: gpio_set_level(IN4, 0)
    
    GPIO->>Hardware: 设置引脚电平
    Hardware->>Hardware: 电机开始转动
    
    Motor->>Motor: vTaskDelay(2000ms)
    
    Motor->>Motor: motor_stop()
    Motor->>GPIO: 所有引脚设为0
    GPIO->>Hardware: 停止信号
    Hardware->>Hardware: 电机停止
    
    Motor-->>API: ESP_OK
    API-->>Web: {"success": true}
    Web-->>User: 显示执行完成
```

## 8. 部署架构图

### 8.1 开发环境架构

```
开发环境部署架构:
┌─────────────────────────────────────────────────────────────────┐
│                       开发主机                                  │
├─────────────────┬─────────────────┬─────────────────┬─────────────────┤
│   ESP-IDF       │    VS Code      │     Git         │    工具链       │
│   v5.2          │   + 插件        │   版本控制      │  xtensa-gcc     │
└─────────────────┴─────────────────┴─────────────────┴─────────────────┘
                                  │
                                  ▼ USB/UART
┌─────────────────────────────────────────────────────────────────┐
│                      ESP32-S3开发板                             │
├─────────────────┬─────────────────┬─────────────────┬─────────────────┤
│   调试接口      │   程序烧录      │   串口监控      │   性能分析      │
│   JTAG/SWD     │   esptool.py    │  idf.py monitor │   heap/task     │
└─────────────────┴─────────────────┴─────────────────┴─────────────────┘
                                  │
                                  ▼ WiFi
┌─────────────────────────────────────────────────────────────────┐
│                       测试环境                                  │
├─────────────────┬─────────────────┬─────────────────┬─────────────────┤
│   本地路由器    │   测试设备      │   API测试       │   性能监控      │
│   WiFi网络      │  手机/电脑      │   Postman       │   日志分析      │
└─────────────────┴─────────────────┴─────────────────┴─────────────────┘
```

### 8.2 生产环境架构

```
生产环境部署架构:
┌─────────────────────────────────────────────────────────────────┐
│                       云端服务                                  │
├─────────────────┬─────────────────┬─────────────────┬─────────────────┤
│   AI API服务    │   OTA更新       │   数据收集      │   监控告警      │
│  Mistral API    │   固件分发      │   使用统计      │   系统状态      │
└─────────────────┴─────────────────┴─────────────────┴─────────────────┘
                                  │
                                  ▼ Internet
┌─────────────────────────────────────────────────────────────────┐
│                      网络基础设施                               │
├─────────────────┬─────────────────┬─────────────────┬─────────────────┤
│   边缘路由器    │   负载均衡      │   安全网关      │   网络监控      │
│   WiFi接入      │   流量分发      │   防火墙        │   带宽管理      │
└─────────────────┴─────────────────┴─────────────────┴─────────────────┘
                                  │
                                  ▼ WiFi/Ethernet
┌─────────────────────────────────────────────────────────────────┐
│                      ESP32-S3设备集群                           │
├─────────────────┬─────────────────┬─────────────────┬─────────────────┤
│   设备1         │   设备2         │   设备3         │   设备N         │
│  摄像头+AI      │  摄像头+AI      │  摄像头+AI      │  摄像头+AI      │
│  电机控制       │  电机控制       │  电机控制       │  电机控制       │
└─────────────────┴─────────────────┴─────────────────┴─────────────────┘
```

## 9. 安全架构图

### 9.1 安全防护层次

```
安全防护架构:
┌─────────────────────────────────────────────────────────────────┐
│                       应用安全层                                │
├─────────────────┬─────────────────┬─────────────────┬─────────────────┤
│   输入验证      │   权限控制      │   数据加密      │   审计日志      │
│  参数过滤       │  访问限制       │  敏感信息       │  操作记录       │
└─────────────────┴─────────────────┴─────────────────┴─────────────────┘
┌─────────────────────────────────────────────────────────────────┐
│                       网络安全层                                │
├─────────────────┬─────────────────┬─────────────────┬─────────────────┤
│   HTTPS/TLS     │   API限流       │   CORS策略      │   IP白名单      │
│  传输加密       │  防止滥用       │  跨域保护       │  访问控制       │
└─────────────────┴─────────────────┴─────────────────┴─────────────────┘
┌─────────────────────────────────────────────────────────────────┐
│                       系统安全层                                │
├─────────────────┬─────────────────┬─────────────────┬─────────────────┤
│   固件签名      │   安全启动      │   内存保护      │   异常处理      │
│  防篡改        │  可信根         │  栈保护         │  错误恢复       │
└─────────────────┴─────────────────┴─────────────────┴─────────────────┘
┌─────────────────────────────────────────────────────────────────┐
│                       硬件安全层                                │
├─────────────────┬─────────────────┬─────────────────┬─────────────────┤
│   安全芯片      │   物理保护      │   电源管理      │   EMC防护       │
│  加密存储       │  防拆卸         │  过压保护       │  电磁兼容       │
└─────────────────┴─────────────────┴─────────────────┴─────────────────┘
```

## 10. 性能监控架构

### 10.1 监控指标体系

```mermaid
graph TB
    subgraph "系统性能监控"
        CPU[CPU使用率]
        Memory[内存使用]
        Network[网络流量]
        Storage[存储空间]
    end
    
    subgraph "应用性能监控"
        Response[响应时间]
        Throughput[处理吞吐量]
        Error[错误率]
        Availability[可用性]
    end
    
    subgraph "硬件状态监控"
        Temperature[温度]
        Power[功耗]
        Signal[信号强度]
        Hardware[硬件状态]
    end
    
    subgraph "业务指标监控"
        AI_Accuracy[AI准确率]
        Task_Success[任务成功率]
        User_Activity[用户活跃度]
        Device_Health[设备健康度]
    end
    
    subgraph "监控数据汇聚"
        Collector[数据收集器]
        Analyzer[分析引擎]
        Alerter[告警系统]
        Dashboard[监控面板]
    end
    
    CPU --> Collector
    Memory --> Collector
    Response --> Collector
    AI_Accuracy --> Collector
    
    Collector --> Analyzer
    Analyzer --> Alerter
    Analyzer --> Dashboard
```

这份架构文档提供了系统的全方位视角，帮助开发者理解系统的设计理念、组件关系和数据流向。通过这些图表，可以更好地进行系统维护、功能扩展和性能优化。