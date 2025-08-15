#!/bin/bash

# ESP32-S3 MicroPython固件烧录脚本
# 此脚本会将所有必要的文件烧录到正确的地址

set -e

# 配置参数
PORT=${1:-/dev/ttyUSB0}  # 串口，可以作为第一个参数传入
BAUD=${2:-460800}        # 波特率，可以作为第二个参数传入

echo "ESP32-S3 MicroPython固件烧录脚本"
echo "=============================="
echo "串口: $PORT"
echo "波特率: $BAUD"
echo ""

# 检查文件是否存在
FIRMWARE_DIR="$(dirname "$0")/esp32-s3-complete"

if [ ! -d "$FIRMWARE_DIR" ]; then
    echo "错误: 固件目录不存在: $FIRMWARE_DIR"
    exit 1
fi

FILES_TO_CHECK=(
    "$FIRMWARE_DIR/bootloader.bin"
    "$FIRMWARE_DIR/partition-table.bin"
    "$FIRMWARE_DIR/micropython.bin"
)

for file in "${FILES_TO_CHECK[@]}"; do
    if [ ! -f "$file" ]; then
        echo "错误: 固件文件不存在: $file"
        exit 1
    fi
done

echo "检查固件文件..."
echo "✓ bootloader.bin ($(stat -c%s "$FIRMWARE_DIR/bootloader.bin") bytes)"
echo "✓ partition-table.bin ($(stat -c%s "$FIRMWARE_DIR/partition-table.bin") bytes)"
echo "✓ micropython.bin ($(stat -c%s "$FIRMWARE_DIR/micropython.bin") bytes)"
echo ""

# 检查esptool是否可用
if ! command -v esptool.py &> /dev/null; then
    echo "错误: esptool.py未找到，请确保已安装ESP-IDF或esptool"
    echo "安装方法: pip install esptool"
    exit 1
fi

echo "开始烧录固件..."
echo ""

# 烧录命令
esptool.py --chip esp32s3 \
    --port "$PORT" \
    --baud "$BAUD" \
    --before default_reset \
    --after hard_reset \
    write_flash \
    --flash_mode dio \
    --flash_freq 80m \
    --flash_size 8MB \
    0x0 "$FIRMWARE_DIR/bootloader.bin" \
    0x8000 "$FIRMWARE_DIR/partition-table.bin" \
    0x10000 "$FIRMWARE_DIR/micropython.bin"

echo ""
echo "烧录完成！"
echo ""
echo "使用方法："
echo "1. 连接ESP32-S3开发板到电脑"
echo "2. 确认串口设备（通常是/dev/ttyUSB0或/dev/ttyACM0）"
echo "3. 运行: ./flash_esp32s3.sh [串口] [波特率]"
echo "   例如: ./flash_esp32s3.sh /dev/ttyUSB0 460800"
echo ""
echo "烧录后，您可以使用串口工具连接ESP32-S3："
echo "  screen $PORT 115200"
echo "  或者使用idf.py monitor"