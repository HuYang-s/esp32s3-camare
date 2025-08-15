# ESP32-S3 "Invalid image block" 问题解决方案

## 问题描述

在烧录ESP32-S3 MicroPython固件时遇到以下错误：
```
Invalid image block, can't boot.
ets_main.c 329
```

## 问题原因

ESP32-S3启动需要三个关键文件，而不是单一的固件文件：

1. **Bootloader** (0x0地址) - 引导加载程序
2. **Partition Table** (0x8000地址) - 分区表
3. **Application** (0x10000地址) - MicroPython固件

之前只烧录了MicroPython固件到0x0地址，缺少引导加载程序和分区表，导致无法启动。

## 解决方案

### 🚀 快速解决（推荐）

使用我们提供的专用烧录脚本：

```bash
# 方法1: 使用idf.py（最简单）
cd /workspace/esp32-micropython-firmware/firmware
source ../esp/esp-idf/export.sh
./flash_esp32s3_simple.sh /dev/ttyUSB0

# 方法2: 使用esptool.py（手动控制）
./flash_esp32s3.sh /dev/ttyUSB0 460800
```

### 📋 手动烧录方法

如果要手动烧录，需要按以下顺序烧录三个文件：

```bash
esptool.py --chip esp32s3 --port /dev/ttyUSB0 --baud 460800 \
    --before default_reset --after hard_reset \
    write_flash --flash_mode dio --flash_freq 80m --flash_size 8MB \
    0x0 esp32-s3-complete/bootloader.bin \
    0x8000 esp32-s3-complete/partition-table.bin \
    0x10000 esp32-s3-complete/micropython.bin
```

## 文件说明

| 文件 | 地址 | 大小 | 作用 |
|------|------|------|------|
| bootloader.bin | 0x0 | ~19KB | ESP32-S3引导加载程序 |
| partition-table.bin | 0x8000 | 3KB | Flash分区表 |
| micropython.bin | 0x10000 | ~1.5MB | MicroPython固件 |

## 验证烧录成功

烧录完成后，ESP32-S3应该显示类似以下的启动信息：

```
ESP-ROM:esp32s3-20210327
Build:Mar 27 2021
rst:0x1 (POWERON),boot:0x8 (SPI_FAST_FLASH_BOOT)
SPIWP:0xee
mode:DIO, clock div:1
load:0x3c130020,len:0x43bc4
load:0x3fc9cb00,len:0x4bfc
load:0x40374000,len:0x7828
load:0x42000020,len:0x121628
entry 0x40374000
I (24) boot: ESP-IDF v5.2 2nd stage bootloader
I (24) boot: compile time Aug 14 2024 21:29:29
I (24) boot: Multicore bootloader
I (28) boot: chip revision: v0.2
...
MicroPython v1.24.1 on 2024-08-14; Generic ESP32S3 module with ESP32S3
Type "help()" for more information.
>>>
```

## 常见问题

### Q: 为什么ESP32不需要这么复杂的烧录？
A: ESP32和ESP32-S3有不同的启动机制。ESP32-S3需要更复杂的分区表和启动流程。

### Q: 可以合并这三个文件吗？
A: 理论上可以，但需要正确的偏移量。使用分别烧录的方法更安全可靠。

### Q: 烧录后还是无法启动怎么办？
A: 
1. 检查串口连接和波特率
2. 尝试擦除整个Flash：`esptool.py --chip esp32s3 erase_flash`
3. 重新烧录固件
4. 检查开发板是否支持ESP32-S3

## 技术细节

ESP32-S3使用了更先进的启动流程：
1. ROM bootloader 启动
2. 加载 2nd stage bootloader (我们的bootloader.bin)
3. 读取分区表 (partition-table.bin)
4. 根据分区表加载应用程序 (micropython.bin)

这就是为什么需要三个文件的原因。