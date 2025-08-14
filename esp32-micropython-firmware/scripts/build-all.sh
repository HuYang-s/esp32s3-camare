#!/bin/bash

set -e

echo "ESP32-S3 MicroPython固件一键构建脚本"
echo "====================================="

# 获取脚本所在目录的绝对路径
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

echo "步骤1: 设置开发环境..."
"$SCRIPT_DIR/setup.sh"

echo ""
echo "步骤2: 构建固件..."
"$SCRIPT_DIR/build.sh"

echo ""
echo "构建完成！固件已保存在 firmware/ 目录中。"