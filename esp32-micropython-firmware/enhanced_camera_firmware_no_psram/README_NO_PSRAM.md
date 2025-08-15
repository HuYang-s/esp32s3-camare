# ESP32-S3 Enhanced Camera Firmware (No PSRAM Version)
# ESP32-S3 增强相机固件（无PSRAM版本）

## 🚨 PSRAM错误解决方案

如果您在刷写原始固件时遇到以下错误：

```
E (299) quad_psram: PSRAM ID read error: 0x00ffffff, PSRAM chip not found or not supported
E (300) esp_psram: PSRAM enabled but initialization failed. Bailing out.
```

这表明您的ESP32-S3开发板**没有PSRAM**或PSRAM配置不匹配。请使用此无PSRAM版本的固件。

## 📦 包含文件

```
enhanced_camera_firmware_no_psram/
├── esp32s3_bootloader.bin           # 启动加载器（无PSRAM）
├── esp32s3_micropython.bin          # MicroPython固件（无PSRAM）
├── esp32s3_partition_table.bin      # 分区表
├── camera_enhanced.py               # 增强相机模块
├── test_enhanced_camera.py          # 测试脚本
├── flash_firmware_no_psram.sh       # 专用刷写脚本
└── README_NO_PSRAM.md              # 本说明文件
```

## 🔧 使用方法

### 1. 刷写固件

```bash
cd enhanced_camera_firmware_no_psram
./flash_firmware_no_psram.sh /dev/cu.usbserial-110
```

### 2. 上传增强模块

```bash
# 安装ampy（如果还没有）
pip install adafruit-ampy

# 上传增强模块
ampy --port /dev/cu.usbserial-110 put camera_enhanced.py
```

### 3. 测试功能

连接到设备：

```bash
# macOS
screen /dev/cu.usbserial-110 115200

# 或使用picocom
brew install picocom
picocom /dev/cu.usbserial-110 -b 115200
```

在MicroPython REPL中测试：

```python
import camera_enhanced as camera

# 检查状态
status = camera.get_status()
print("Camera status:", status)

# 查看可用预设
presets = camera.list_presets()
print("Available presets:", presets)

# 加载预设配置
camera.load_preset('ESP32-S3-CAM')

# 获取当前引脚配置
pins = camera.get_pins()
print("Current pins:", pins)
```

## ⚠️ 重要说明

### 关于相机功能

**标准ESP32-S3固件可能不包含相机驱动**，因此：

1. **引脚管理功能** ✅ - `camera_enhanced.py`的引脚配置功能仍然可用
2. **相机拍摄功能** ❓ - 取决于固件是否包含相机驱动

### 如果需要完整相机功能

如果您的开发板确实有PSRAM，可能需要：

1. **检查硬件** - 确认您的ESP32-S3开发板型号
2. **使用正确固件** - 寻找与您硬件匹配的固件
3. **自定义编译** - 根据具体硬件配置编译固件

## 🔍 硬件识别

### 常见ESP32-S3开发板类型

| 开发板型号 | PSRAM | 推荐固件 |
|-----------|-------|----------|
| ESP32-S3-DevKitC-1 | 无 | 此无PSRAM版本 ✅ |
| ESP32-S3-DevKitC-1U | 无 | 此无PSRAM版本 ✅ |
| ESP32-S3-DevKitM-1 | 有 | 原始相机固件 |
| ESP32-S3-CAM | 有 | 原始相机固件 |
| AI-Thinker ESP32-S3 | 看型号 | 根据具体型号选择 |

### 如何确认PSRAM

在MicroPython REPL中运行：

```python
import esp
print("Total heap:", esp.heap_size())
print("Free heap:", esp.heap_free())

# 如果总堆内存 > 400KB，通常表示有PSRAM
```

## 🛠️ 故障排除

### 1. 固件刷写失败

```bash
# 尝试更低的波特率
./flash_firmware_no_psram.sh /dev/cu.usbserial-110 115200

# 或手动刷写
esptool.py --chip esp32s3 --port /dev/cu.usbserial-110 --baud 115200 write_flash -z \
  0x0 esp32s3_bootloader.bin \
  0x8000 esp32s3_partition_table.bin \
  0x10000 esp32s3_micropython.bin
```

### 2. 设备无法连接

- 确保设备处于下载模式（按住BOOT按钮）
- 检查USB线缆和驱动
- 尝试不同的波特率

### 3. 模块上传失败

```bash
# 尝试重置设备后再上传
ampy --port /dev/cu.usbserial-110 --delay 1 put camera_enhanced.py
```

## 📞 技术支持

如果仍有问题：

1. 检查您的ESP32-S3开发板具体型号
2. 确认硬件是否有PSRAM芯片
3. 考虑使用原厂推荐的固件
4. 查看开发板厂商的技术文档

---

**注意**: 此版本固件专为**无PSRAM的ESP32-S3开发板**设计。如果您的开发板有PSRAM，请使用相应的固件版本。