# ESP32-S3相机模块完整指南

## 🎯 概述

本指南将帮助您在ESP32-S3上使用MicroPython相机模块，包括固件构建、烧录和使用。

## 📋 前提条件

### 硬件要求
- ESP32-S3开发板
- OV2640/OV3660/OV5640等兼容相机模块
- 稳定的USB连接

### 软件要求
- Linux环境（Ubuntu推荐）
- ESP-IDF v5.2环境（已在项目中配置）
- Python 3.8+
- esptool.py

## 🛠️ 构建相机固件

### 方法1: 使用自动化脚本（推荐）

```bash
# 进入固件目录
cd /workspace/esp32-micropython-firmware/firmware_final_with_camera_instructions

# 构建相机固件
./build_camera_firmware.sh
```

脚本将自动：
1. 激活ESP-IDF环境
2. 检查相机驱动
3. 修复配置问题
4. 构建包含相机模块的固件
5. 复制固件文件到当前目录

### 方法2: 手动构建

```bash
# 1. 激活ESP-IDF环境
cd /workspace/esp32-micropython-firmware
source esp/esp-idf/export.sh

# 2. 进入MicroPython ESP32端口目录
cd esp/micropython/ports/esp32

# 3. 清理之前的构建
make clean

# 4. 构建相机固件
make USER_C_MODULES="/workspace/esp32-micropython-firmware/esp/micropython/examples/usercmodule/cam/micropython.cmake"
```

## 🔥 烧录相机固件

### 使用脚本烧录

```bash
# 确保ESP32-S3已连接到电脑
./flash_esp32s3_camera.sh /dev/ttyUSB0 460800
```

### 手动烧录

```bash
esptool.py --chip esp32s3 --port /dev/ttyUSB0 --baud 460800 \
    --before default_reset --after hard_reset \
    write_flash --flash_mode dio --flash_freq 80m --flash_size 8MB \
    0x0 esp32s3_camera_bootloader.bin \
    0x8000 esp32s3_camera_partition_table.bin \
    0x10000 esp32s3_camera_micropython.bin
```

## 📷 相机模块使用

### 基础使用

```python
import camera

# 初始化相机
camera.init()

# 拍照
img = camera.capture()

# 获取图像信息
print(f"图像大小: {len(img)} bytes")

# 释放相机
camera.deinit()
```

### 高级配置

```python
import camera

# 配置相机参数
camera.init(
    format=camera.JPEG,      # 图像格式
    framesize=camera.FRAME_VGA,  # 分辨率
    quality=10               # JPEG质量 (10-63, 数字越小质量越高)
)

# 设置其他参数
camera.set_brightness(0)     # 亮度 (-2 到 2)
camera.set_contrast(0)       # 对比度 (-2 到 2)
camera.set_saturation(0)     # 饱和度 (-2 到 2)

# 拍照
img = camera.capture()
```

### 保存图像到文件

```python
import camera

camera.init()
img = camera.capture()

# 保存到文件
with open('photo.jpg', 'wb') as f:
    f.write(img)

print("图像已保存为 photo.jpg")
```

### 网络传输示例

```python
import camera
import socket
import network

# 连接WiFi
wlan = network.WLAN(network.STA_IF)
wlan.active(True)
wlan.connect('your-wifi', 'your-password')

# 等待连接
while not wlan.isconnected():
    pass

# 初始化相机
camera.init(framesize=camera.FRAME_QVGA, quality=15)

# 创建HTTP服务器
def handle_request(conn):
    request = conn.recv(1024).decode()
    
    if '/capture' in request:
        # 拍照
        img = camera.capture()
        
        # 发送HTTP响应
        response = f"""HTTP/1.1 200 OK
Content-Type: image/jpeg
Content-Length: {len(img)}

"""
        conn.send(response.encode())
        conn.send(img)
    else:
        # 发送简单的HTML页面
        html = """
        <html>
        <body>
            <h1>ESP32-S3 Camera</h1>
            <img src="/capture" width="320" height="240">
            <br><br>
            <a href="/capture">Capture Photo</a>
        </body>
        </html>
        """
        response = f"""HTTP/1.1 200 OK
Content-Type: text/html

{html}"""
        conn.send(response.encode())

# 启动服务器
addr = socket.getaddrinfo('0.0.0.0', 80)[0][-1]
s = socket.socket()
s.bind(addr)
s.listen(1)

print(f'相机服务器启动: http://{wlan.ifconfig()[0]}/')

while True:
    conn, addr = s.accept()
    try:
        handle_request(conn)
    finally:
        conn.close()
```

## 🔧 故障排除

### 常见问题

#### 1. 构建失败
```
错误: jpeg_decoder.h: No such file or directory
```
**解决方案**: 运行自动构建脚本，它会自动修复路径问题。

#### 2. 相机初始化失败
```python
>>> import camera
>>> camera.init()
OSError: Camera init failed
```
**解决方案**:
- 检查相机模块连接
- 确认使用兼容的相机模块（OV2640推荐）
- 检查引脚连接是否正确

#### 3. PSRAM错误
```
E (287) esp_psram: PSRAM enabled but initialization failed
```
**解决方案**: 这是警告信息，不影响相机功能。如果需要PSRAM支持，检查开发板是否支持。

#### 4. 内存不足
```python
>>> img = camera.capture()
MemoryError: memory allocation failed
```
**解决方案**:
- 降低图像分辨率：`camera.init(framesize=camera.FRAME_QVGA)`
- 提高JPEG压缩：`camera.init(quality=30)`
- 及时释放图像数据

### 调试技巧

```python
# 检查相机状态
import camera
print("相机模块可用:", hasattr(camera, 'init'))

# 获取相机信息
camera.init()
print("相机已初始化")

# 测试小尺寸图像
camera.init(framesize=camera.FRAME_QQVGA, quality=30)
img = camera.capture()
print(f"测试图像大小: {len(img)} bytes")
```

## 📊 性能优化

### 内存优化
- 使用较低的分辨率进行测试
- 及时释放图像数据
- 避免在循环中累积大量图像

### 速度优化
- 使用合适的JPEG质量设置
- 预先配置相机参数
- 使用异步方式处理图像

## 🎯 实际项目示例

### 1. 定时拍照器
```python
import camera
import time
import machine

camera.init(framesize=camera.FRAME_VGA, quality=12)

for i in range(10):
    print(f"拍摄第 {i+1} 张照片...")
    img = camera.capture()
    
    filename = f"photo_{i+1:02d}.jpg"
    with open(filename, 'wb') as f:
        f.write(img)
    
    print(f"已保存: {filename} ({len(img)} bytes)")
    time.sleep(5)

camera.deinit()
print("拍摄完成！")
```

### 2. 运动检测
```python
import camera
import time

camera.init(framesize=camera.FRAME_QVGA, quality=20)

# 拍摄基准图像
baseline = camera.capture()
print("基准图像已设置")

while True:
    current = camera.capture()
    
    # 简单的变化检测（比较文件大小差异）
    size_diff = abs(len(current) - len(baseline))
    threshold = 1000  # 调整此值来设置敏感度
    
    if size_diff > threshold:
        print(f"检测到运动! 差异: {size_diff} bytes")
        # 保存图像
        with open('motion_detected.jpg', 'wb') as f:
            f.write(current)
        baseline = current  # 更新基准
    
    time.sleep(1)
```

## 📚 API参考

### camera模块函数

| 函数 | 描述 | 参数 |
|------|------|------|
| `camera.init()` | 初始化相机 | format, framesize, quality |
| `camera.capture()` | 拍摄照片 | 无 |
| `camera.deinit()` | 释放相机 | 无 |
| `camera.set_brightness()` | 设置亮度 | -2 到 2 |
| `camera.set_contrast()` | 设置对比度 | -2 到 2 |
| `camera.set_saturation()` | 设置饱和度 | -2 到 2 |

### 常量

```python
# 图像格式
camera.JPEG
camera.RGB565

# 分辨率
camera.FRAME_QQVGA    # 160x120
camera.FRAME_QVGA     # 320x240
camera.FRAME_VGA      # 640x480
camera.FRAME_SVGA     # 800x600
camera.FRAME_XGA      # 1024x768
camera.FRAME_SXGA     # 1280x1024
camera.FRAME_UXGA     # 1600x1200
```

---
*更新时间: $(date)*
*ESP-IDF版本: v5.2*
*MicroPython版本: v1.24.1*