#!/bin/bash

set -e

echo "开始构建ESP32-S3 MicroPython固件..."

# 获取脚本所在目录的绝对路径
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"
ESP_DIR="$PROJECT_ROOT/esp"
FIRMWARE_DIR="$PROJECT_ROOT/firmware"

echo "项目根目录: $PROJECT_ROOT"
echo "ESP目录: $ESP_DIR"
echo "固件输出目录: $FIRMWARE_DIR"

# 检查环境是否已设置
if [ ! -d "$ESP_DIR/esp-idf" ]; then
    echo "错误: ESP-IDF未找到，请先运行 ./scripts/setup.sh"
    exit 1
fi

if [ ! -d "$ESP_DIR/micropython" ]; then
    echo "错误: MicroPython未找到，请先运行 ./scripts/setup.sh"
    exit 1
fi

# 激活ESP-IDF环境
echo "激活ESP-IDF环境..."
source "$ESP_DIR/esp-idf/export.sh"

# 验证ESP-IDF版本
echo "验证ESP-IDF版本..."
idf.py --version

# 构建基础MicroPython固件
echo "步骤1: 构建基础MicroPython固件..."
cd "$ESP_DIR/micropython"
make -C mpy-cross

cd ports/esp32
make submodules
make

# 复制基础固件
if [ -f "build-ESP32_GENERIC/firmware.bin" ]; then
    cp "build-ESP32_GENERIC/firmware.bin" "$FIRMWARE_DIR/esp32-generic-firmware.bin"
    echo "基础ESP32固件已保存到: $FIRMWARE_DIR/esp32-generic-firmware.bin"
else
    echo "警告: 基础ESP32固件构建失败"
fi

# 设置ESP32-S3相机固件构建
echo "步骤2: 设置ESP32-S3相机驱动..."

# 下载相机驱动到ESP-IDF components
cd "$ESP_DIR/esp-idf/components"
if [ ! -d "esp32-camera" ]; then
    git clone https://github.com/espressif/esp32-camera
fi

# 下载MicroPython相机驱动
cd "$ESP_DIR/micropython/ports/esp32"
if [ ! -d "micropython-camera-driver" ]; then
    git clone https://github.com/lemariva/micropython-camera-driver
fi

# 创建cam模块目录并复制文件
mkdir -p "$ESP_DIR/micropython/examples/usercmodule/cam"
cp micropython-camera-driver/src/micropython.cmake "$ESP_DIR/micropython/examples/usercmodule/cam/"
cp micropython-camera-driver/src/micropython.mk "$ESP_DIR/micropython/examples/usercmodule/cam/"
cp micropython-camera-driver/src/modcamera.c "$ESP_DIR/micropython/examples/usercmodule/cam/"
cp micropython-camera-driver/src/modcamera.h "$ESP_DIR/micropython/examples/usercmodule/cam/"

# 修改Makefile以支持ESP32-S3
echo "步骤3: 配置ESP32-S3构建..."
cd "$ESP_DIR/micropython/ports/esp32"
cp Makefile Makefile.backup
sed -i 's/BOARD ?= ESP32_GENERIC/BOARD ?= ESP32_GENERIC_S3/' Makefile

# 修改相机模块以默认启用
echo "步骤4: 修改相机模块配置..."
MODCAMERA_FILE="$ESP_DIR/micropython/examples/usercmodule/cam/modcamera.c"
if [ -f "$MODCAMERA_FILE" ]; then
    # 创建修改后的modcamera.c
    cp "$MODCAMERA_FILE" "${MODCAMERA_FILE}.backup"
    
    # 删除条件编译指令并修改模块注册
    sed -i '/^#if MODULE_CAMERA_ENABLED/d' "$MODCAMERA_FILE"
    sed -i '/^#endif$/d' "$MODCAMERA_FILE"
    sed -i 's/MP_REGISTER_MODULE(MP_QSTR_camera, mp_module_camera_system, MODULE_CAMERA_ENABLED);/MP_REGISTER_MODULE(MP_QSTR_camera, mp_module_camera_system);/' "$MODCAMERA_FILE"
    
    echo "相机模块配置已修改"
fi

# 构建ESP32-S3相机固件
echo "步骤5: 构建ESP32-S3相机固件..."
make clean
make USER_C_MODULES="$ESP_DIR/micropython/examples/usercmodule/cam/micropython.cmake"

# 复制ESP32-S3固件
if [ -f "build-ESP32_GENERIC_S3/firmware.bin" ]; then
    cp "build-ESP32_GENERIC_S3/firmware.bin" "$FIRMWARE_DIR/esp32-s3-camera-firmware.bin"
    echo "ESP32-S3相机固件已保存到: $FIRMWARE_DIR/esp32-s3-camera-firmware.bin"
else
    echo "错误: ESP32-S3相机固件构建失败"
    exit 1
fi

echo ""
echo "固件构建完成！"
echo "生成的固件文件:"
ls -la "$FIRMWARE_DIR"/*.bin 2>/dev/null || echo "未找到固件文件"

echo ""
echo "固件说明:"
echo "- esp32-generic-firmware.bin: 基础ESP32 MicroPython固件"
echo "- esp32-s3-camera-firmware.bin: ESP32-S3相机功能MicroPython固件"