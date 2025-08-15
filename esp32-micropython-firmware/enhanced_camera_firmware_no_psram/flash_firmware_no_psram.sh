#!/bin/bash

# ESP32-S3 Enhanced Camera Firmware Flash Script (No PSRAM)
# ESP32-S3 增强相机固件刷写脚本（无PSRAM版本）

set -e

# Default values
PORT="/dev/ttyUSB0"
BAUD="460800"
CHIP="esp32s3"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

print_info() {
    echo -e "${GREEN}[INFO]${NC} $1"
}

print_warning() {
    echo -e "${YELLOW}[WARNING]${NC} $1"
}

print_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

show_help() {
    echo "ESP32-S3 Enhanced Camera Firmware Flash Script (No PSRAM)"
    echo "ESP32-S3 增强相机固件刷写脚本（无PSRAM版本）"
    echo ""
    echo "Usage: $0 [PORT] [BAUD] [CHIP]"
    echo ""
    echo "Parameters:"
    echo "  PORT    Serial port (default: /dev/ttyUSB0)"
    echo "  BAUD    Baud rate (default: 460800)"
    echo "  CHIP    Chip type (default: esp32s3)"
    echo ""
    echo "Examples:"
    echo "  $0                              # Use defaults"
    echo "  $0 /dev/cu.usbserial-110        # macOS"
    echo "  $0 /dev/ttyUSB1 921600          # Custom port and baud"
    echo ""
    echo "Note: This version is for ESP32-S3 boards WITHOUT PSRAM"
    echo "注意：此版本适用于不带PSRAM的ESP32-S3开发板"
}

# Parse arguments
if [ "$1" = "-h" ] || [ "$1" = "--help" ]; then
    show_help
    exit 0
fi

if [ ! -z "$1" ]; then
    PORT="$1"
fi

if [ ! -z "$2" ]; then
    BAUD="$2"
fi

if [ ! -z "$3" ]; then
    CHIP="$3"
fi

# Get script directory
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

print_info "ESP32-S3 增强相机固件刷写工具（无PSRAM版本）"
print_info "Enhanced Camera Firmware Flash Tool (No PSRAM Version)"
echo ""

# Check if esptool.py is available
if ! command -v esptool.py &> /dev/null; then
    print_error "esptool.py 未找到。请安装 esptool"
    print_error "esptool.py not found. Please install esptool"
    echo ""
    echo "安装方法 (Installation):"
    echo "pip install esptool"
    exit 1
fi

# Check firmware files
BOOTLOADER="esp32s3_bootloader.bin"
PARTITION_TABLE="esp32s3_partition_table.bin"
MICROPYTHON="esp32s3_micropython.bin"

for file in "$BOOTLOADER" "$PARTITION_TABLE" "$MICROPYTHON"; do
    if [ ! -f "$file" ]; then
        print_error "固件文件未找到: $file"
        print_error "Firmware file not found: $file"
        exit 1
    fi
done

print_info "验证固件文件..."
print_info "Verifying firmware files..."

# Get file sizes for verification
BOOTLOADER_SIZE=$(wc -c < "$BOOTLOADER" | tr -d ' ')
MICROPYTHON_SIZE=$(wc -c < "$MICROPYTHON" | tr -d ' ')
PARTITION_SIZE=$(wc -c < "$PARTITION_TABLE" | tr -d ' ')

print_info "固件文件信息:"
print_info "Firmware file info:"
echo "  Bootloader: $BOOTLOADER_SIZE bytes"
echo "  MicroPython: $MICROPYTHON_SIZE bytes"
echo "  Partition Table: $PARTITION_SIZE bytes"

print_info "配置信息:"
print_info "Configuration:"
echo "  芯片类型 (Chip): $CHIP"
echo "  串口 (Port): $PORT"
echo "  波特率 (Baud): $BAUD"
echo "  PSRAM: 禁用 (Disabled)"
echo ""

print_warning "重要提示: 此固件适用于不带PSRAM的ESP32-S3开发板"
print_warning "Important: This firmware is for ESP32-S3 boards WITHOUT PSRAM"
echo ""

print_info "开始刷写固件..."
print_info "Starting firmware flash..."

# Flash firmware - note the different addresses for standard ESP32-S3
esptool.py --chip "$CHIP" --port "$PORT" --baud "$BAUD" write_flash -z \
    0x0 "$BOOTLOADER" \
    0x8000 "$PARTITION_TABLE" \
    0x10000 "$MICROPYTHON"

if [ $? -eq 0 ]; then
    print_info "固件刷写成功！"
    print_info "Firmware flashed successfully!"
    echo ""
    print_info "下一步:"
    print_info "Next steps:"
    echo "1. 上传 camera_enhanced.py 到设备"
    echo "   Upload camera_enhanced.py to device:"
    echo "   ampy --port $PORT put camera_enhanced.py"
    echo ""
    echo "2. 重启设备并测试功能"
    echo "   Restart device and test functionality:"
    echo "   import camera_enhanced as camera"
    echo "   camera.get_status()"
    echo ""
    print_warning "注意: 标准ESP32-S3固件可能不包含相机驱动"
    print_warning "Note: Standard ESP32-S3 firmware may not include camera driver"
    print_info "但您仍可以使用camera_enhanced.py进行引脚管理"
    print_info "But you can still use camera_enhanced.py for pin management"
else
    print_error "固件刷写失败"
    print_error "Firmware flash failed"
    exit 1
fi