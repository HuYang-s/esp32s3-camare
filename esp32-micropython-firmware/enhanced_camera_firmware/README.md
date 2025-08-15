# ESP32-S3 Enhanced Camera Firmware

This package contains the enhanced MicroPython firmware for ESP32-S3 with dynamic camera pin configuration support.

## 文件说明 (File Description)

### 固件文件 (Firmware Files)
- `esp32s3_camera_bootloader.bin` - 启动加载器 (19KB)
- `esp32s3_camera_micropython.bin` - MicroPython固件主体 (1.6MB) 
- `esp32s3_camera_partition_table.bin` - 分区表 (3KB)

### 增强模块 (Enhanced Module)
- `camera_enhanced.py` - 动态引脚配置模块 (Dynamic pin configuration module)

### 文档 (Documentation)
- `CAMERA_GUIDE.md` - 相机使用指南 (Camera usage guide)
- `flash_firmware.sh` - 固件刷写脚本 (Firmware flashing script)

## 固件校验和 (Firmware Checksums)

```
f2ca02bd721ddb2812ef9afb49da9e00bd94eb5f6d331d4286c0e55b8f77af75  esp32s3_camera_bootloader.bin
3bbcfc7ec45e95afd50c732e3e1669e006c7c927bd908bbcbe6ebe59cd7b71ea  esp32s3_camera_micropython.bin
79d4d11acdbdad3cc82dc25b98b90c77f42eb70bf0db064bc6884e8bcc42e331  esp32s3_camera_partition_table.bin
```

## 快速开始 (Quick Start)

### 1. 刷写固件 (Flash Firmware)

```bash
# 使用esptool刷写固件
chmod +x flash_firmware.sh
./flash_firmware.sh /dev/ttyUSB0

# 或手动刷写
esptool.py --chip esp32s3 --port /dev/ttyUSB0 --baud 460800 write_flash -z \
  0x0 esp32s3_camera_bootloader.bin \
  0x8000 esp32s3_camera_partition_table.bin \
  0x10000 esp32s3_camera_micropython.bin
```

### 2. 上传增强模块 (Upload Enhanced Module)

将 `camera_enhanced.py` 上传到ESP32-S3的文件系统中：

```python
# 使用ampy或其他工具上传
ampy --port /dev/ttyUSB0 put camera_enhanced.py
```

### 3. 使用动态引脚配置 (Use Dynamic Pin Configuration)

```python
import camera_enhanced as camera

# 方法1：使用预设配置
camera.load_preset('ESP32-S3-CAM')

# 方法2：自定义引脚配置
custom_pins = {
    'xclk': 15,
    'siod': 4,
    'sioc': 5,
    'y9': 16,
    'y8': 17,
    'y7': 18,
    'y6': 12,
    'y5': 10,
    'y4': 8,
    'y3': 9,
    'y2': 11,
    'vsync': 6,
    'href': 7,
    'pclk': 13,
    'pwdn': 43,
    'reset': 44
}
camera.set_pins(custom_pins)

# 初始化相机
camera.init()

# 拍照
buf = camera.capture()
print(f"拍摄照片，大小: {len(buf)} 字节")
```

## 支持的开发板 (Supported Boards)

预设配置包括以下开发板：
- ESP32-S3-CAM
- ESP32-CAM (AI-Thinker)
- TTGO T-Camera
- M5Stack Camera

## 特性 (Features)

1. **动态引脚配置** - 运行时修改相机GPIO引脚
2. **预设配置** - 支持常见开发板的预设引脚配置
3. **配置保存/加载** - 将自定义配置保存到文件系统
4. **错误处理** - 完善的错误检查和异常处理
5. **兼容性** - 与原生camera模块API完全兼容

## 技术实现 (Technical Implementation)

该增强模块通过以下方式实现动态引脚配置：

1. **Python包装器** - 包装原生camera模块
2. **machine.Pin配置** - 使用machine.Pin在初始化前配置GPIO
3. **运行时重映射** - 在MicroPython层面实现引脚重映射
4. **无需重编译** - 无需自定义C固件，使用标准固件即可

## 故障排除 (Troubleshooting)

### 常见问题

1. **相机初始化失败**
   ```python
   camera.test_camera()  # 运行测试
   ```

2. **引脚冲突**
   ```python
   camera.get_status()  # 检查当前状态
   ```

3. **配置问题**
   ```python
   camera.list_presets()  # 查看可用预设
   ```

## 版本信息 (Version Info)

- MicroPython 固件版本: v1.23.0 (基于ESP-IDF v5.2)
- 增强模块版本: v1.0.0
- 构建日期: 2024年

## 许可证 (License)

本项目遵循MIT许可证。详见项目根目录的LICENSE文件。