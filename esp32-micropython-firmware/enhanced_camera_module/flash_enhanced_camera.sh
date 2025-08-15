#!/bin/bash

# Enhanced ESP32-S3相机固件烧录脚本
# 支持动态引脚配置的相机固件烧录

set -e

# 默认参数
DEFAULT_PORT="/dev/ttyUSB0"
DEFAULT_BAUD="460800"

# 获取参数
PORT=${1:-$DEFAULT_PORT}
BAUD=${2:-$DEFAULT_BAUD}

echo "Enhanced ESP32-S3相机固件烧录脚本"
echo "================================="
echo "支持动态引脚配置的相机固件"
echo ""
echo "串口: $PORT"
echo "波特率: $BAUD"
echo ""

# 检查串口设备
if [ ! -e "$PORT" ]; then
    echo "错误: 串口设备 $PORT 不存在"
    echo "请检查ESP32-S3是否已连接，或使用正确的串口设备"
    echo ""
    echo "常见串口设备:"
    echo "  Linux: /dev/ttyUSB0, /dev/ttyACM0"
    echo "  macOS: /dev/cu.usbserial-*"
    echo "  Windows: COM3, COM4"
    exit 1
fi

# 检查固件文件
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BOOTLOADER="$SCRIPT_DIR/esp32s3_enhanced_camera_bootloader.bin"
PARTITION_TABLE="$SCRIPT_DIR/esp32s3_enhanced_camera_partition_table.bin"
FIRMWARE="$SCRIPT_DIR/esp32s3_enhanced_camera_micropython.bin"

echo "检查固件文件..."
if [ ! -f "$BOOTLOADER" ]; then
    echo "错误: 找不到bootloader文件: $BOOTLOADER"
    echo "请先运行构建脚本: ./build_enhanced_firmware.sh"
    exit 1
fi

if [ ! -f "$PARTITION_TABLE" ]; then
    echo "错误: 找不到分区表文件: $PARTITION_TABLE"
    echo "请先运行构建脚本: ./build_enhanced_firmware.sh"
    exit 1
fi

if [ ! -f "$FIRMWARE" ]; then
    echo "错误: 找不到固件文件: $FIRMWARE"
    echo "请先运行构建脚本: ./build_enhanced_firmware.sh"
    exit 1
fi

echo "✓ 所有固件文件已找到"

# 显示文件信息
echo ""
echo "固件信息:"
echo "  Bootloader: $(basename "$BOOTLOADER") ($(stat -c%s "$BOOTLOADER") bytes)"
echo "  分区表: $(basename "$PARTITION_TABLE") ($(stat -c%s "$PARTITION_TABLE") bytes)"
echo "  固件: $(basename "$FIRMWARE") ($(stat -c%s "$FIRMWARE") bytes)"
echo ""

# 检查esptool.py
if ! command -v esptool.py &> /dev/null; then
    echo "错误: esptool.py 未找到"
    echo "请安装esptool: pip install esptool"
    exit 1
fi

echo "开始烧录..."
echo "注意: 请确保ESP32-S3处于下载模式（按住BOOT键，然后按RST键）"
echo ""

# 执行烧录
esptool.py --chip esp32s3 --port "$PORT" --baud "$BAUD" \
    --before default_reset --after hard_reset \
    write_flash --flash_mode dio --flash_freq 80m --flash_size 8MB \
    0x0 "$BOOTLOADER" \
    0x8000 "$PARTITION_TABLE" \
    0x10000 "$FIRMWARE"

if [ $? -eq 0 ]; then
    echo ""
    echo "🎉 固件烧录成功！"
    echo ""
    echo "增强功能特性:"
    echo "  ✓ 动态引脚配置支持"
    echo "  ✓ 改进的错误处理"
    echo "  ✓ 更灵活的相机初始化"
    echo "  ✓ 引脚状态查询功能"
    echo ""
    echo "下一步:"
    echo "1. 重启ESP32-S3设备"
    echo "2. 连接到MicroPython REPL"
    echo "3. 测试新的相机功能"
    echo ""
    echo "快速测试命令:"
    echo "  import camera"
    echo "  print(camera.get_pins())  # 查看当前引脚配置"
    echo "  camera.init()             # 使用默认配置初始化"
    echo "  img = camera.capture()    # 拍照"
    echo "  print(f'图像大小: {len(img)} bytes')"
    echo ""
    echo "自定义引脚配置示例:"
    echo "  pins = {"
    echo "    'xclk': 0, 'sda': 26, 'scl': 27,"
    echo "    'd7': 35, 'd6': 34, 'd5': 39, 'd4': 36,"
    echo "    'd3': 21, 'd2': 19, 'd1': 18, 'd0': 5,"
    echo "    'vsync': 25, 'href': 23, 'pclk': 22"
    echo "  }"
    echo "  camera.set_pins(pins)"
    echo "  camera.init()"
else
    echo ""
    echo "❌ 烧录失败"
    echo ""
    echo "常见解决方案:"
    echo "1. 检查ESP32-S3是否正确连接"
    echo "2. 确认设备处于下载模式"
    echo "3. 检查串口权限: sudo chmod 666 $PORT"
    echo "4. 尝试不同的波特率: 115200"
    echo "5. 检查USB线是否支持数据传输"
    exit 1
fi