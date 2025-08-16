# ESP32-S3摄像头AI分析系统 - API接口文档

## 概述

本文档详细描述了ESP32-S3摄像头AI分析系统的所有API接口，包括请求格式、响应格式、错误码和使用示例。

### 基础信息
- **基础URL**: `http://<ESP32_IP_ADDRESS>`
- **端口**: 80 (HTTP)
- **内容类型**: application/json
- **字符编码**: UTF-8

### 通用响应格式
所有API响应都遵循统一的格式：

```json
{
    "success": boolean,      // 操作是否成功
    "message": string,       // 成功消息（可选）
    "error": string,         // 错误消息（失败时）
    "data": object          // 响应数据（可选）
}
```

### 错误处理
- **400 Bad Request**: 请求格式错误或参数无效
- **404 Not Found**: 请求的资源不存在
- **500 Internal Server Error**: 服务器内部错误
- **503 Service Unavailable**: 服务暂时不可用

## API端点详情

### 1. 主页面
获取系统的Web管理界面。

```http
GET /
```

**响应**:
- Content-Type: text/html
- 返回完整的HTML页面，包含：
  - 摄像头图像展示
  - 电机控制面板
  - AI任务管理界面
  - 系统状态监控

### 2. 电机控制API

#### 2.1 基础电机控制
控制电机的基本运动（前进、后退、左转、右转、停止）。

```http
POST /api/motor
Content-Type: application/json
```

**请求参数**:
```json
{
    "action": "forward|backward|left|right|stop",
    "speed": 255,        // 可选，电机速度 (0-255，默认255)
    "duration": 1000     // 可选，持续时间毫秒 (默认1000)
}
```

**请求示例**:
```json
{
    "action": "forward",
    "speed": 200,
    "duration": 2000
}
```

**响应示例**:
```json
{
    "success": true,
    "message": "电机控制命令已执行"
}
```

**错误响应**:
```json
{
    "success": false,
    "error": "无效的电机控制命令"
}
```

#### 2.2 精确角度控制
控制电机按指定角度转向。

```http
POST /api/motor/angle
Content-Type: application/json
```

**请求参数**:
```json
{
    "angle": float,      // 转向角度（正值右转，负值左转）
    "speed": 255         // 可选，转向速度 (0-255)
}
```

**请求示例**:
```json
{
    "angle": 45.0,       // 右转45度
    "speed": 180
}
```

**响应示例**:
```json
{
    "success": true,
    "message": "电机角度控制完成",
    "data": {
        "executed_angle": 45.0,
        "execution_time_ms": 250
    }
}
```

#### 2.3 差速驱动控制
独立控制左右电机速度实现精确移动。

```http
POST /api/motor/differential
Content-Type: application/json
```

**请求参数**:
```json
{
    "left_speed": int,   // 左电机速度 (-255到255)
    "right_speed": int,  // 右电机速度 (-255到255)
    "duration": 1000     // 持续时间毫秒
}
```

**请求示例**:
```json
{
    "left_speed": 200,   // 左轮前转
    "right_speed": -100, // 右轮后转（实现原地左转）
    "duration": 1500
}
```

### 3. AI任务控制API

#### 3.1 启动/停止AI搜索任务
管理AI物体搜索任务的生命周期。

```http
POST /api/ai-task
Content-Type: application/json
```

**启动任务请求**:
```json
{
    "action": "start",
    "target": "laptop",     // 搜索目标物体名称
    "timeout": 30,          // 超时时间（秒）
    "use_navigation": false // 可选，是否启用导航搜索
}
```

**停止任务请求**:
```json
{
    "action": "stop"
}
```

**成功响应**:
```json
{
    "success": true,
    "message": "AI搜索任务已启动",
    "data": {
        "task_id": "search_20241216_143022",
        "target": "laptop",
        "timeout": 30,
        "start_time": "2024-12-16T14:30:22Z"
    }
}
```

**支持的搜索目标**:
- 人物: person
- 交通工具: car, bicycle, motorcycle, bus, truck, boat
- 动物: cat, dog, bird, horse, cow, sheep
- 电子设备: laptop, tv, cell phone, keyboard, mouse, remote
- 家具: chair, couch, bed, dining table, toilet
- 日用品: bottle, cup, bowl, book, clock, vase, scissors
- 食物: apple, banana, orange, pizza, cake, donut
- 等等...（支持COCO数据集的80个类别）

#### 3.2 查询AI任务状态
获取当前AI任务的执行状态和结果。

```http
GET /api/ai-task-status
```

**响应示例**:
```json
{
    "status": "searching",           // 任务状态
    "target": "laptop",             // 搜索目标
    "progress": 0.75,               // 进度 (0.0-1.0)
    "message": "正在搜索目标物体...", // 状态消息
    "timeout": 30,                  // 超时时间
    "elapsed_time": 22,             // 已用时间（秒）
    "result_count": 1,              // 检测结果数量
    "results": [                    // 检测结果数组
        {
            "class_name": "laptop",
            "confidence": 0.85,
            "x": 0.3,              // 边界框左上角X坐标 (0-1)
            "y": 0.4,              // 边界框左上角Y坐标 (0-1)
            "width": 0.4,          // 边界框宽度 (0-1)
            "height": 0.3          // 边界框高度 (0-1)
        }
    ],
    "search_cycles": 15,           // 搜索周期数
    "areas_explored": 3,           // 已探索区域数
    "detailed_log": "搜索日志..."  // 详细执行日志
}
```

**任务状态说明**:
- `idle`: 空闲状态，无任务执行
- `searching`: 正在搜索目标物体
- `completed`: 任务完成，找到目标
- `failed_timeout`: 任务超时失败
- `failed_unable`: 任务执行失败

#### 3.3 AI导航搜索任务
启动带有主动导航功能的AI搜索任务。

```http
POST /api/ai-navigation-task
Content-Type: application/json
```

**请求参数**:
```json
{
    "action": "start|stop",
    "target": "person",        // 搜索目标
    "timeout": 60,            // 超时时间（秒）
    "search_strategy": "systematic|random|spiral", // 搜索策略
    "max_areas": 5            // 最大搜索区域数
}
```

**响应示例**:
```json
{
    "success": true,
    "message": "AI导航搜索任务已启动",
    "data": {
        "navigation_enabled": true,
        "search_strategy": "systematic",
        "estimated_duration": 45
    }
}
```

### 4. AI自动驾驶API

#### 4.1 控制自动驾驶模式
启用或禁用AI自动驾驶功能。

```http
POST /api/ai-auto-drive
Content-Type: application/json
```

**请求参数**:
```json
{
    "enabled": true,          // 是否启用自动驾驶
    "sensitivity": 0.8,       // 可选，AI决策敏感度 (0.1-1.0)
    "speed_limit": 200        // 可选，最大速度限制 (0-255)
}
```

**响应示例**:
```json
{
    "success": true,
    "enabled": true,
    "message": "AI自动驾驶已启用",
    "data": {
        "current_mode": "auto_drive",
        "sensitivity": 0.8,
        "speed_limit": 200,
        "last_decision": "forward",
        "decision_confidence": 0.92
    }
}
```

#### 4.2 查询自动驾驶状态
获取当前自动驾驶的状态信息。

```http
GET /api/ai-auto-drive/status
```

**响应示例**:
```json
{
    "enabled": true,
    "current_action": "forward",
    "confidence": 0.88,
    "obstacles_detected": false,
    "last_decision_time": "2024-12-16T14:35:22Z",
    "total_decisions": 156,
    "success_rate": 0.94
}
```

### 5. 图像管理API

#### 5.1 获取图像列表
获取系统中保存的所有图像信息。

```http
GET /api/images
```

**查询参数**:
- `limit`: 返回数量限制（默认50）
- `offset`: 偏移量（默认0）
- `sort`: 排序方式（time_desc|time_asc|size_desc|size_asc）

**请求示例**:
```http
GET /api/images?limit=20&offset=0&sort=time_desc
```

**响应示例**:
```json
{
    "success": true,
    "data": {
        "total": 127,
        "images": [
            {
                "filename": "20241216_143022.jpg",
                "timestamp": "2024-12-16 14:30:22",
                "size": 45623,
                "width": 800,
                "height": 600,
                "ai_result": "检测到: laptop (置信度: 0.85)",
                "analysis_type": "local_ai",
                "has_detection": true,
                "url": "/api/images/20241216_143022.jpg"
            },
            {
                "filename": "20241216_142955.jpg",
                "timestamp": "2024-12-16 14:29:55",
                "size": 42156,
                "width": 800,
                "height": 600,
                "ai_result": "未检测到物体",
                "analysis_type": "cloud_ai",
                "has_detection": false,
                "url": "/api/images/20241216_142955.jpg"
            }
        ]
    }
}
```

#### 5.2 获取单个图像
获取指定的图像文件。

```http
GET /api/images/{filename}
```

**响应**:
- Content-Type: image/jpeg
- 返回JPEG格式的图像数据

#### 5.3 删除图像
删除指定的图像文件。

```http
DELETE /api/images/{filename}
```

**响应示例**:
```json
{
    "success": true,
    "message": "图像已删除"
}
```

### 6. 系统状态API

#### 6.1 获取系统状态
获取系统的整体运行状态。

```http
GET /api/system/status
```

**响应示例**:
```json
{
    "success": true,
    "data": {
        "uptime": 3600,                    // 运行时间（秒）
        "free_heap": 145632,               // 可用内存（字节）
        "min_free_heap": 128456,           // 最小可用内存
        "wifi_status": "connected",        // WiFi状态
        "wifi_rssi": -45,                  // WiFi信号强度
        "camera_status": "ready",          // 摄像头状态
        "motor_status": "ready",           // 电机状态
        "ai_service_status": "ready",      // AI服务状态
        "storage_usage": {
            "total": 327680,               // 总存储空间
            "used": 156432,                // 已用空间
            "available": 171248            // 可用空间
        },
        "task_info": {
            "total_tasks": 5,
            "running_tasks": 5,
            "cpu_usage": 0.65              // CPU使用率
        }
    }
}
```

#### 6.2 获取网络信息
获取网络连接信息。

```http
GET /api/system/network
```

**响应示例**:
```json
{
    "success": true,
    "data": {
        "sta_mode": {
            "enabled": true,
            "connected": true,
            "ssid": "MyWiFi",
            "ip": "192.168.1.100",
            "gateway": "192.168.1.1",
            "netmask": "255.255.255.0",
            "rssi": -45
        },
        "ap_mode": {
            "enabled": true,
            "ssid": "ESP32-CAM-AI",
            "ip": "192.168.4.1",
            "connected_clients": 2
        }
    }
}
```

#### 6.3 重启系统
重启ESP32系统。

```http
POST /api/system/restart
```

**响应示例**:
```json
{
    "success": true,
    "message": "系统将在3秒后重启"
}
```

### 7. 配置管理API

#### 7.1 获取系统配置
获取当前系统配置。

```http
GET /api/config
```

**响应示例**:
```json
{
    "success": true,
    "data": {
        "camera": {
            "resolution": "SVGA",          // 分辨率
            "quality": 12,                 // 图像质量 (0-63)
            "brightness": 0,               // 亮度 (-2到2)
            "contrast": 0,                 // 对比度 (-2到2)
            "saturation": 0                // 饱和度 (-2到2)
        },
        "ai": {
            "local_ai_enabled": true,
            "cloud_ai_enabled": true,
            "detection_threshold": 0.5,    // 检测阈值
            "max_detections": 10           // 最大检测数量
        },
        "motor": {
            "default_speed": 255,
            "turn_angle_ratio": 5.56,      // 角度转换比例
            "acceleration": 100            // 加速度
        },
        "network": {
            "ap_ssid": "ESP32-CAM-AI",
            "ap_password": "12345678",
            "sta_auto_connect": true
        }
    }
}
```

#### 7.2 更新系统配置
更新系统配置参数。

```http
PUT /api/config
Content-Type: application/json
```

**请求示例**:
```json
{
    "camera": {
        "quality": 10,
        "brightness": 1
    },
    "ai": {
        "detection_threshold": 0.6
    }
}
```

**响应示例**:
```json
{
    "success": true,
    "message": "配置已更新",
    "data": {
        "updated_fields": ["camera.quality", "camera.brightness", "ai.detection_threshold"],
        "restart_required": false
    }
}
```

## WebSocket接口

### 实时状态推送
系统支持WebSocket连接以获取实时状态更新。

```javascript
const ws = new WebSocket('ws://192.168.1.100/ws');

ws.onmessage = function(event) {
    const data = JSON.parse(event.data);
    console.log('收到实时数据:', data);
};
```

**推送数据格式**:
```json
{
    "type": "status_update",
    "timestamp": "2024-12-16T14:30:22Z",
    "data": {
        "ai_task_progress": 0.65,
        "motor_status": "moving_forward",
        "detection_results": [...],
        "system_metrics": {...}
    }
}
```

## 错误码参考

| 错误码 | 说明 | 解决方案 |
|--------|------|----------|
| 1001 | 摄像头初始化失败 | 检查摄像头连接 |
| 1002 | 电机驱动错误 | 检查L298N连接 |
| 1003 | WiFi连接失败 | 检查网络配置 |
| 1004 | AI服务不可用 | 检查网络连接和API密钥 |
| 1005 | 存储空间不足 | 清理图像文件 |
| 1006 | 任务执行超时 | 增加超时时间或检查系统负载 |
| 1007 | 参数验证失败 | 检查请求参数格式 |
| 1008 | 资源冲突 | 等待当前任务完成 |

## 使用示例

### JavaScript示例

```javascript
// 控制电机前进
async function moveForward() {
    try {
        const response = await fetch('/api/motor', {
            method: 'POST',
            headers: {
                'Content-Type': 'application/json'
            },
            body: JSON.stringify({
                action: 'forward',
                duration: 2000
            })
        });
        
        const result = await response.json();
        if (result.success) {
            console.log('电机控制成功');
        } else {
            console.error('电机控制失败:', result.error);
        }
    } catch (error) {
        console.error('请求失败:', error);
    }
}

// 启动AI搜索任务
async function startAISearch(target) {
    try {
        const response = await fetch('/api/ai-task', {
            method: 'POST',
            headers: {
                'Content-Type': 'application/json'
            },
            body: JSON.stringify({
                action: 'start',
                target: target,
                timeout: 30
            })
        });
        
        const result = await response.json();
        if (result.success) {
            console.log('AI搜索任务已启动');
            // 开始轮询状态
            pollTaskStatus();
        }
    } catch (error) {
        console.error('启动搜索失败:', error);
    }
}

// 轮询任务状态
async function pollTaskStatus() {
    const interval = setInterval(async () => {
        try {
            const response = await fetch('/api/ai-task-status');
            const result = await response.json();
            
            console.log('任务状态:', result.status);
            console.log('进度:', result.progress);
            
            if (result.status === 'completed' || 
                result.status === 'failed_timeout' || 
                result.status === 'failed_unable') {
                clearInterval(interval);
                console.log('任务结束:', result.message);
            }
        } catch (error) {
            console.error('状态查询失败:', error);
            clearInterval(interval);
        }
    }, 1000);
}
```

### Python示例

```python
import requests
import time
import json

class ESP32CameraAPI:
    def __init__(self, base_url):
        self.base_url = base_url.rstrip('/')
    
    def move_motor(self, action, speed=255, duration=1000):
        """控制电机移动"""
        url = f"{self.base_url}/api/motor"
        data = {
            "action": action,
            "speed": speed,
            "duration": duration
        }
        
        response = requests.post(url, json=data)
        return response.json()
    
    def start_ai_search(self, target, timeout=30):
        """启动AI搜索任务"""
        url = f"{self.base_url}/api/ai-task"
        data = {
            "action": "start",
            "target": target,
            "timeout": timeout
        }
        
        response = requests.post(url, json=data)
        return response.json()
    
    def get_task_status(self):
        """获取任务状态"""
        url = f"{self.base_url}/api/ai-task-status"
        response = requests.get(url)
        return response.json()
    
    def wait_for_task_completion(self, poll_interval=1):
        """等待任务完成"""
        while True:
            status = self.get_task_status()
            print(f"任务状态: {status['status']}, 进度: {status.get('progress', 0):.2%}")
            
            if status['status'] in ['completed', 'failed_timeout', 'failed_unable']:
                return status
            
            time.sleep(poll_interval)

# 使用示例
if __name__ == "__main__":
    api = ESP32CameraAPI("http://192.168.1.100")
    
    # 控制电机
    result = api.move_motor("forward", speed=200, duration=2000)
    print("电机控制结果:", result)
    
    # 启动AI搜索
    result = api.start_ai_search("laptop", timeout=60)
    if result['success']:
        print("搜索任务已启动")
        
        # 等待任务完成
        final_status = api.wait_for_task_completion()
        print("最终结果:", final_status)
```

## 最佳实践

### 1. 错误处理
- 始终检查响应的`success`字段
- 实现重试机制处理网络错误
- 记录错误信息便于调试

### 2. 性能优化
- 使用WebSocket获取实时更新而非频繁轮询
- 合理设置请求超时时间
- 批量处理多个操作

### 3. 安全考虑
- 在生产环境中启用HTTPS
- 实现API访问限制和身份验证
- 验证和清理输入参数

### 4. 资源管理
- 及时停止不需要的AI任务
- 定期清理旧的图像文件
- 监控系统内存使用情况

这份API文档提供了系统所有接口的详细说明，开发者可以根据需要选择合适的接口来构建自己的应用程序。