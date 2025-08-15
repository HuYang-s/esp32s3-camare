#!/bin/bash

# ESP32-S3 Enhanced Camera Firmware Flash Script
# 增强相机固件刷写脚本

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
    echo "ESP32-S3 Enhanced Camera Firmware Flash Script"
    echo ""
    echo "Usage: $0 [PORT] [BAUD] [CHIP]"
    echo ""
    echo "Parameters:"
    echo "  PORT    Serial port (default: /dev/ttyUSB0)"
    echo "  BAUD    Baud rate (default: 460800)"
    echo "  CHIP    Chip type (default: esp32s3)"
    echo ""
    echo "Examples:"
    echo "  $0                          # Use defaults"
    echo "  $0 /dev/ttyUSB1             # Custom port"
    echo "  $0 /dev/ttyUSB1 921600      # Custom port and baud"
    echo ""
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

print_info "ESP32-S3 增强相机固件刷写工具"
print_info "Enhanced Camera Firmware Flash Tool"
echo ""

# Check if esptool.py is available
if ! command -v esptool.py &> /dev/null; then
    print_error "esptool.py 未找到。请安装 ESP-IDF 或 esptool"
    print_error "esptool.py not found. Please install ESP-IDF or esptool"
    echo ""
    echo "安装方法 (Installation):"
    echo "pip install esptool"
    exit 1
fi

# Check firmware files
BOOTLOADER="esp32s3_camera_bootloader.bin"
PARTITION_TABLE="esp32s3_camera_partition_table.bin"
MICROPYTHON="esp32s3_camera_micropython.bin"

for file in "$BOOTLOADER" "$PARTITION_TABLE" "$MICROPYTHON"; do
    if [ ! -f "$file" ]; then
        print_error "固件文件未找到: $file"
        print_error "Firmware file not found: $file"
        exit 1
    fi
done

print_info "验证固件文件..."
print_info "Verifying firmware files..."

# Verify checksums
EXPECTED_CHECKSUMS="f2ca02bd721ddb2812ef9afb49da9e00bd94eb5f6d331d4286c0e55b8f77af75  $BOOTLOADER
3bbcfc7ec45e95afd50c732e3e1669e006c7c927bd908bbcbe6ebe59cd7b71ea  $MICROPYTHON
79d4d11acdbdad3cc82dc25b98b90c77f42eb70bf0db064bc6884e8bcc42e331  $PARTITION_TABLE"

if command -v sha256sum &> /dev/null; then
    echo "$EXPECTED_CHECKSUMS" | sha256sum -c --quiet
    if [ $? -eq 0 ]; then
        print_info "固件文件校验成功"
        print_info "Firmware files verified successfully"
    else
        print_warning "固件文件校验失败，但继续刷写"
        print_warning "Firmware file verification failed, but continuing"
    fi
else
    print_warning "sha256sum 不可用，跳过校验"
    print_warning "sha256sum not available, skipping verification"
fi

print_info "配置信息:"
print_info "Configuration:"
echo "  芯片类型 (Chip): $CHIP"
echo "  串口 (Port): $PORT"
echo "  波特率 (Baud): $BAUD"
echo ""

print_info "开始刷写固件..."
print_info "Starting firmware flash..."

# Flash firmware
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
    echo "   Upload camera_enhanced.py to device"
    echo "2. 重启设备并测试相机功能"
    echo "   Restart device and test camera functionality"
    echo ""
    echo "测试命令 (Test command):"
    echo "import camera_enhanced as camera"
    echo "camera.test_camera()"
else
    print_error "固件刷写失败"
    print_error "Firmware flash failed"
    exit 1
fi