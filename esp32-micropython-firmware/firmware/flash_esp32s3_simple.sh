#!/bin/bash

# ESP32-S3 简化烧录脚本 (使用idf.py)
# 这个脚本使用ESP-IDF的idf.py工具进行烧录

set -e

PORT=${1:-/dev/ttyUSB0}

echo "ESP32-S3 简化烧录脚本"
echo "===================="
echo "串口: $PORT"
echo ""

# 检查ESP-IDF环境
if ! command -v idf.py &> /dev/null; then
    echo "错误: idf.py未找到，请先激活ESP-IDF环境："
    echo "source ../esp/esp-idf/export.sh"
    exit 1
fi

# 进入构建目录
BUILD_DIR="../esp/micropython/ports/esp32/build-ESP32_GENERIC_S3"

if [ ! -d "$BUILD_DIR" ]; then
    echo "错误: 构建目录不存在: $BUILD_DIR"
    echo "请先运行构建脚本生成固件"
    exit 1
fi

echo "使用idf.py烧录固件..."
cd "$BUILD_DIR"

# 使用idf.py flash命令，它会自动处理所有文件和地址
idf.py -p "$PORT" flash

echo ""
echo "烧录完成！"
echo ""
echo "现在您可以连接串口查看输出："
echo "  idf.py -p $PORT monitor"
echo "或者使用其他串口工具："
echo "  screen $PORT 115200"