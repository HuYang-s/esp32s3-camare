# 固件信息

## 构建成功！🎉

本项目已成功构建了ESP32和ESP32-S3的MicroPython固件。

### 生成的固件文件

1. **esp32-generic-firmware.bin**
   - 大小: 1,675,072 bytes (1.6MB)
   - 目标芯片: ESP32
   - MicroPython版本: v1.24.1
   - ESP-IDF版本: v5.2
   - 功能: 基础MicroPython固件，包含标准模块

2. **esp32-s3-generic-firmware.bin**
   - 大小: 1,583,408 bytes (1.5MB)
   - 目标芯片: ESP32-S3
   - MicroPython版本: v1.24.1
   - ESP-IDF版本: v5.2
   - 功能: 基础MicroPython固件，包含标准模块

## 固件烧录方法

⚠️ **重要提示**: ESP32-S3需要烧录多个文件到不同地址，不能只烧录单一固件文件！

### 方法1: 使用专用烧录脚本（推荐）

#### ESP32-S3固件烧录:
```bash
# 方法1a: 使用idf.py（推荐，自动处理所有细节）
source ../esp/esp-idf/export.sh
./flash_esp32s3_simple.sh /dev/ttyUSB0

# 方法1b: 使用esptool.py（手动指定所有文件）
./flash_esp32s3.sh /dev/ttyUSB0 460800
```

#### ESP32固件烧录:
```bash
python -m esptool --chip esp32 --port /dev/ttyUSB0 --baud 460800 write_flash -z 0x1000 esp32-generic-firmware.bin
```

### 方法2: 手动使用esptool.py烧录ESP32-S3

ESP32-S3需要烧录三个文件：
```bash
esptool.py --chip esp32s3 --port /dev/ttyUSB0 --baud 460800 \
    --before default_reset --after hard_reset \
    write_flash --flash_mode dio --flash_freq 80m --flash_size 8MB \
    0x0 esp32-s3-complete/bootloader.bin \
    0x8000 esp32-s3-complete/partition-table.bin \
    0x10000 esp32-s3-complete/micropython.bin
```

### 使用idf.py烧录 (如果在ESP-IDF环境中)
```bash
# 激活ESP-IDF环境
source ../esp/esp-idf/export.sh

# ESP32
idf.py -p /dev/ttyUSB0 flash

# ESP32-S3  
idf.py -p /dev/ttyUSB0 flash
```

## 固件特性

### 包含的模块
- 标准MicroPython模块 (os, sys, time, json等)
- 网络模块 (network, socket, ssl)
- 蓝牙模块 (bluetooth)
- WiFi功能
- GPIO、SPI、I2C、UART等硬件接口
- 文件系统支持 (FAT, LittleFS)

### 内存配置
- Python堆内存: ~100KB
- 内置文件系统: 2MB (ESP32) / 6MB (ESP32-S3)

## 测试方法

烧录固件后，通过串口连接到开发板（波特率115200），应该能看到MicroPython REPL：

```
>>> print("Hello MicroPython!")
Hello MicroPython!
>>> import sys
>>> sys.implementation
(name='micropython', version=(1, 24, 1))
```

## 注意事项

1. **相机功能**: 由于依赖兼容性问题，本次构建的固件暂不包含相机模块。如需相机功能，可能需要：
   - 使用兼容的ESP32相机驱动版本
   - 或等待MicroPython相机驱动更新

2. **端口配置**: 烧录时请根据实际连接的串口设备修改端口号（如/dev/ttyUSB0、COM3等）

3. **波特率**: 建议使用460800波特率以获得更快的烧录速度

## 构建环境信息

- 构建时间: 2024年8月14日
- 系统: Ubuntu 25.04
- ESP-IDF: v5.2
- MicroPython: v1.24.1
- 编译器: xtensa-esp-elf-gcc 13.2.0