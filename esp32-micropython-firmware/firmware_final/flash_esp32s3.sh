#!/bin/bash

# ESP32-S3 MicroPython固件烧录脚本

set -e

PORT=${1:-/dev/ttyUSB0}
BAUD=${2:-460800}

echo "ESP32-S3 MicroPython固件烧录"
echo "=========================="
echo "串口: $PORT"
echo "波特率: $BAUD"
echo ""

# 检查文件
FILES=(
    "esp32s3_bootloader.bin"
    "esp32s3_partition_table.bin"
    "esp32s3_micropython.bin"
)

for file in "${FILES[@]}"; do
    if [ ! -f "$file" ]; then
        echo "错误: 文件不存在: $file"
        exit 1
    fi
done

echo "检查固件文件..."
echo "✓ esp32s3_bootloader.bin ($(stat -c%s esp32s3_bootloader.bin) bytes)"
echo "✓ esp32s3_partition_table.bin ($(stat -c%s esp32s3_partition_table.bin) bytes)"
echo "✓ esp32s3_micropython.bin ($(stat -c%s esp32s3_micropython.bin) bytes)"
echo ""

# 检查esptool
if ! command -v esptool.py &> /dev/null; then
    echo "错误: esptool.py未找到"
    echo "安装方法: pip install esptool"
    exit 1
fi

echo "开始烧录ESP32-S3固件..."
esptool.py --chip esp32s3 \
    --port "$PORT" \
    --baud "$BAUD" \
    --before default_reset \
    --after hard_reset \
    write_flash \
    --flash_mode dio \
    --flash_freq 80m \
    --flash_size 8MB \
    0x0 esp32s3_bootloader.bin \
    0x8000 esp32s3_partition_table.bin \
    0x10000 esp32s3_micropython.bin

echo ""
echo "ESP32-S3固件烧录完成！"
echo ""
echo "现在您可以连接串口查看输出："
echo "  screen $PORT 115200"