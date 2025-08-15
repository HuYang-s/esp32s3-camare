# Enhanced ESP32-S3 相机模块

## 🎯 概述

这是一个增强的ESP32-S3 MicroPython相机模块，支持动态引脚配置和高级相机功能。与原始固件相比，此模块提供了更灵活的引脚配置选项，允许在MicroPython代码中动态设置相机引脚，而无需重新编译固件。

## ✨ 新功能特性

- ✅ **动态引脚配置**: 在MicroPython中灵活设置相机引脚
- ✅ **引脚状态查询**: 查看当前的引脚配置
- ✅ **改进的错误处理**: 更详细的错误信息和异常处理
- ✅ **向后兼容**: 完全兼容原有的相机API
- ✅ **多种引脚配置方式**: 支持字典和列表两种配置方式
- ✅ **预设配置**: 内置常见开发板的引脚配置

## 🛠️ 构建和烧录

### 快速开始

```bash
# 1. 进入增强模块目录
cd /workspace/esp32-micropython-firmware/enhanced_camera_module

# 2. 构建增强固件
./build_enhanced_firmware.sh

# 3. 烧录到ESP32-S3设备
./flash_enhanced_camera.sh /dev/ttyUSB0
```

### 详细步骤

1. **环境准备**
   ```bash
   # 确保ESP-IDF环境已设置
   cd /workspace/esp32-micropython-firmware
   source esp/esp-idf/export.sh
   ```

2. **构建固件**
   ```bash
   cd enhanced_camera_module
   ./build_enhanced_firmware.sh
   ```

3. **烧录固件**
   ```bash
   # 使用默认串口和波特率
   ./flash_enhanced_camera.sh
   
   # 或指定串口和波特率
   ./flash_enhanced_camera.sh /dev/ttyUSB0 460800
   ```

## 📚 API 文档

### 基本函数

#### `camera.init(**kwargs)`
初始化相机，支持多种参数配置。

**参数:**
- `format`: 图像格式 (默认: `camera.JPEG`)
- `framesize`: 分辨率 (默认: `camera.FRAME_VGA`)
- `quality`: JPEG质量 1-63，数字越小质量越高 (默认: 10)
- `pins`: 引脚配置 (可选)

**示例:**
```python
import camera

# 使用默认配置
camera.init()

# 自定义参数
camera.init(
    format=camera.JPEG,
    framesize=camera.FRAME_QVGA,
    quality=15
)

# 初始化时指定引脚配置
pins = [32, -1, 0, 26, 27, 35, 34, 39, 36, 21, 19, 18, 5, 25, 23, 22]
camera.init(pins=pins)
```

#### `camera.capture()`
拍摄照片，返回图像数据。

**返回值:** 图像字节数据

**示例:**
```python
img = camera.capture()
print(f"图像大小: {len(img)} bytes")
```

#### `camera.deinit()`
释放相机资源。

**示例:**
```python
camera.deinit()
```

### 引脚配置函数

#### `camera.set_pins(pin_config)`
设置相机引脚配置。

**参数:**
- `pin_config`: 字典格式的引脚配置

**引脚名称:**
- `pwdn`: 电源控制引脚
- `reset`: 复位引脚 (-1表示不使用)
- `xclk`: 时钟引脚
- `sda`: I2C数据引脚
- `scl`: I2C时钟引脚
- `d7`-`d0`: 数据引脚 (D7-D0)
- `vsync`: 垂直同步引脚
- `href`: 水平参考引脚
- `pclk`: 像素时钟引脚

**示例:**
```python
# 字典方式配置引脚
pins = {
    'pwdn': 32,
    'reset': -1,
    'xclk': 0,
    'sda': 26,
    'scl': 27,
    'd7': 35,
    'd6': 34,
    'd5': 39,
    'd4': 36,
    'd3': 21,
    'd2': 19,
    'd1': 18,
    'd0': 5,
    'vsync': 25,
    'href': 23,
    'pclk': 22
}

camera.set_pins(pins)
camera.init()
```

#### `camera.get_pins()`
获取当前的引脚配置。

**返回值:** 字典格式的引脚配置

**示例:**
```python
current_pins = camera.get_pins()
print("当前引脚配置:", current_pins)
```

### 相机设置函数

#### `camera.set_brightness(brightness)`
设置相机亮度。

**参数:**
- `brightness`: 亮度值 (-2 到 2)

#### `camera.set_contrast(contrast)`
设置相机对比度。

**参数:**
- `contrast`: 对比度值 (-2 到 2)

#### `camera.set_saturation(saturation)`
设置相机饱和度。

**参数:**
- `saturation`: 饱和度值 (-2 到 2)

**示例:**
```python
camera.init()
camera.set_brightness(1)   # 增加亮度
camera.set_contrast(0)     # 默认对比度
camera.set_saturation(-1)  # 降低饱和度
```

### 常量

#### 图像格式
- `camera.JPEG`: JPEG格式
- `camera.RGB565`: RGB565格式
- `camera.GRAYSCALE`: 灰度格式

#### 分辨率
- `camera.FRAME_QQVGA`: 160x120
- `camera.FRAME_QVGA`: 320x240
- `camera.FRAME_VGA`: 640x480
- `camera.FRAME_SVGA`: 800x600
- `camera.FRAME_XGA`: 1024x768
- `camera.FRAME_SXGA`: 1280x1024
- `camera.FRAME_UXGA`: 1600x1200

## 🔧 使用示例

### 基本使用
```python
import camera

# 初始化相机
camera.init()

# 拍照
img = camera.capture()
print(f"拍照成功，图像大小: {len(img)} bytes")

# 保存到文件
with open('photo.jpg', 'wb') as f:
    f.write(img)

# 释放资源
camera.deinit()
```

### 自定义引脚配置
```python
import camera

# 方法1: 在初始化时指定引脚
custom_pins = {
    'xclk': 0,
    'sda': 26,
    'scl': 27,
    'd7': 35,
    'd6': 34,
    'd5': 39,
    'd4': 36,
    'd3': 21,
    'd2': 19,
    'd1': 18,
    'd0': 5,
    'vsync': 25,
    'href': 23,
    'pclk': 22,
    'pwdn': 32,
    'reset': -1
}

camera.set_pins(custom_pins)
camera.init()

# 方法2: 查看当前配置
print("当前引脚配置:", camera.get_pins())

# 拍照测试
img = camera.capture()
print(f"使用自定义引脚拍照成功: {len(img)} bytes")

camera.deinit()
```

### 高级配置示例
```python
import camera

# 配置引脚
pins = {
    'pwdn': 32, 'reset': -1, 'xclk': 0,
    'sda': 26, 'scl': 27,
    'd7': 35, 'd6': 34, 'd5': 39, 'd4': 36,
    'd3': 21, 'd2': 19, 'd1': 18, 'd0': 5,
    'vsync': 25, 'href': 23, 'pclk': 22
}

camera.set_pins(pins)

# 初始化相机（高质量JPEG，VGA分辨率）
camera.init(
    format=camera.JPEG,
    framesize=camera.FRAME_VGA,
    quality=8  # 高质量
)

# 调整相机参数
camera.set_brightness(0)   # 标准亮度
camera.set_contrast(1)     # 增加对比度
camera.set_saturation(0)   # 标准饱和度

# 拍照
img = camera.capture()
print(f"高质量图像: {len(img)} bytes")

# 保存图像
with open('high_quality.jpg', 'wb') as f:
    f.write(img)

camera.deinit()
```

### 连续拍照示例
```python
import camera
import time

# 初始化
camera.init(framesize=camera.FRAME_QVGA, quality=15)

try:
    for i in range(5):
        print(f"拍照 {i+1}/5...")
        img = camera.capture()
        
        # 保存文件
        filename = f"photo_{i+1:02d}.jpg"
        with open(filename, 'wb') as f:
            f.write(img)
        
        print(f"已保存: {filename} ({len(img)} bytes)")
        time.sleep(2)
        
except Exception as e:
    print(f"拍照过程出错: {e}")
    
finally:
    camera.deinit()
```

### 网络相机服务器
```python
import camera
import socket
import network

# 连接WiFi
wlan = network.WLAN(network.STA_IF)
wlan.active(True)
wlan.connect('your-wifi-ssid', 'your-wifi-password')

while not wlan.isconnected():
    pass

print(f"WiFi连接成功: {wlan.ifconfig()[0]}")

# 配置相机
camera.init(framesize=camera.FRAME_QVGA, quality=20)

# 创建HTTP服务器
def handle_request(conn):
    try:
        request = conn.recv(1024).decode()
        
        if '/capture' in request:
            # 拍照并返回图像
            img = camera.capture()
            
            response = f"""HTTP/1.1 200 OK
Content-Type: image/jpeg
Content-Length: {len(img)}
Connection: close

"""
            conn.send(response.encode())
            conn.send(img)
        else:
            # 返回简单的HTML页面
            html = """<!DOCTYPE html>
<html>
<head>
    <title>ESP32-S3 Enhanced Camera</title>
    <meta charset="utf-8">
</head>
<body>
    <h1>ESP32-S3 增强相机</h1>
    <p>支持动态引脚配置的相机模块</p>
    <img src="/capture" width="320" height="240" alt="Camera Feed">
    <br><br>
    <button onclick="location.reload()">刷新图像</button>
    <br><br>
    <a href="/capture">直接访问图像</a>
</body>
</html>"""
            
            response = f"""HTTP/1.1 200 OK
Content-Type: text/html; charset=utf-8
Content-Length: {len(html.encode('utf-8'))}
Connection: close

{html}"""
            conn.send(response.encode('utf-8'))
            
    except Exception as e:
        print(f"请求处理错误: {e}")

# 启动服务器
addr = socket.getaddrinfo('0.0.0.0', 80)[0][-1]
s = socket.socket()
s.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
s.bind(addr)
s.listen(1)

print(f'相机服务器启动: http://{wlan.ifconfig()[0]}/')

try:
    while True:
        conn, addr = s.accept()
        print(f'客户端连接: {addr}')
        handle_request(conn)
        conn.close()
except KeyboardInterrupt:
    print("服务器停止")
finally:
    s.close()
    camera.deinit()
```

## 🔧 预设引脚配置

### ESP32-S3-CAM (默认)
```python
esp32s3_cam_pins = {
    'pwdn': 32, 'reset': -1, 'xclk': 0,
    'sda': 26, 'scl': 27,
    'd7': 35, 'd6': 34, 'd5': 39, 'd4': 36,
    'd3': 21, 'd2': 19, 'd1': 18, 'd0': 5,
    'vsync': 25, 'href': 23, 'pclk': 22
}
```

### AI-Thinker ESP32-CAM
```python
ai_thinker_pins = {
    'pwdn': 32, 'reset': -1, 'xclk': 0,
    'sda': 26, 'scl': 27,
    'd7': 35, 'd6': 34, 'd5': 39, 'd4': 36,
    'd3': 21, 'd2': 19, 'd1': 18, 'd0': 5,
    'vsync': 25, 'href': 23, 'pclk': 22
}
```

### 自定义配置模板
```python
custom_pins = {
    'pwdn': -1,    # 电源控制引脚 (可选, -1表示不使用)
    'reset': -1,   # 复位引脚 (可选, -1表示不使用)
    'xclk': 0,     # 时钟引脚
    'sda': 26,     # I2C数据引脚
    'scl': 27,     # I2C时钟引脚
    'd7': 35,      # 数据引脚D7
    'd6': 34,      # 数据引脚D6
    'd5': 39,      # 数据引脚D5
    'd4': 36,      # 数据引脚D4
    'd3': 21,      # 数据引脚D3
    'd2': 19,      # 数据引脚D2
    'd1': 18,      # 数据引脚D1
    'd0': 5,       # 数据引脚D0
    'vsync': 25,   # 垂直同步引脚
    'href': 23,    # 水平参考引脚
    'pclk': 22     # 像素时钟引脚
}
```

## 🐛 故障排除

### 常见问题

#### 1. 相机初始化失败
```python
>>> camera.init()
OSError: Camera init failed
```

**解决方案:**
- 检查引脚连接是否正确
- 确认相机模块兼容性 (推荐OV2640)
- 检查电源供应是否充足
- 验证引脚配置是否正确

#### 2. 引脚配置错误
```python
>>> camera.set_pins({'invalid_pin': 1})
TypeError: Unknown pin name
```

**解决方案:**
- 使用正确的引脚名称
- 参考预设配置模板
- 检查引脚是否被其他功能占用

#### 3. 内存不足
```python
>>> img = camera.capture()
MemoryError: memory allocation failed
```

**解决方案:**
- 降低分辨率: `camera.init(framesize=camera.FRAME_QVGA)`
- 增加JPEG压缩: `camera.init(quality=30)`
- 及时释放图像数据
- 使用较小的帧缓冲

#### 4. 图像质量问题

**解决方案:**
- 调整JPEG质量参数 (1-63)
- 优化光照条件
- 调整相机设置 (亮度、对比度、饱和度)
- 检查镜头是否清洁

### 调试技巧

```python
# 1. 检查模块是否正确加载
import camera
print("相机模块版本:", hasattr(camera, 'set_pins'))

# 2. 查看当前引脚配置
print("当前引脚:", camera.get_pins())

# 3. 测试小尺寸图像
camera.init(framesize=camera.FRAME_QQVGA, quality=30)
img = camera.capture()
print(f"测试图像大小: {len(img)} bytes")

# 4. 检查相机状态
try:
    camera.init()
    print("相机初始化成功")
    img = camera.capture()
    print(f"拍照成功: {len(img)} bytes")
except Exception as e:
    print(f"相机错误: {e}")
finally:
    camera.deinit()
```

## 📄 更新日志

### v1.0.0 (Enhanced)
- ✅ 新增动态引脚配置功能
- ✅ 新增引脚状态查询API
- ✅ 改进错误处理和异常信息
- ✅ 支持字典和列表两种配置方式
- ✅ 向后兼容原有API
- ✅ 增强的构建和烧录脚本

## 🤝 贡献

欢迎提交Issue和Pull Request来改进这个项目！

## 📜 许可证

本项目遵循以下开源许可证：
- ESP-IDF: Apache License 2.0
- MicroPython: MIT License