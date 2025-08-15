#!/bin/bash

# ESP32-S3相机固件构建脚本
# 此脚本将构建包含相机模块的ESP32-S3 MicroPython固件

set -e

echo "ESP32-S3相机固件构建脚本"
echo "========================"
echo ""

# 检查是否在正确的目录
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"

if [ ! -d "$PROJECT_ROOT/esp/esp-idf" ]; then
    echo "错误: ESP-IDF未找到，请确保在正确的项目目录中运行此脚本"
    exit 1
fi

echo "项目根目录: $PROJECT_ROOT"
echo ""

# 激活ESP-IDF环境
echo "步骤1: 激活ESP-IDF环境..."
cd "$PROJECT_ROOT"
source esp/esp-idf/export.sh

# 检查相机驱动
echo "步骤2: 检查相机驱动..."
CAMERA_MODULE_DIR="$PROJECT_ROOT/esp/micropython/examples/usercmodule/cam"
if [ ! -f "$CAMERA_MODULE_DIR/modcamera.c" ]; then
    echo "错误: 相机模块未找到，请先运行setup脚本"
    exit 1
fi

echo "✓ 相机模块已准备就绪"

# 修复包含路径问题
echo "步骤3: 修复相机模块配置..."
CMAKE_FILE="$CAMERA_MODULE_DIR/micropython.cmake"

# 备份原文件
cp "$CMAKE_FILE" "$CMAKE_FILE.backup"

# 创建修复后的CMake配置
cat > "$CMAKE_FILE" << 'EOF'
# Create an INTERFACE library for our C module.
add_library(usermod_esp32camera INTERFACE)

# Add our source files to the lib
target_sources(usermod_esp32camera INTERFACE
    ${CMAKE_CURRENT_LIST_DIR}/modcamera.c
)

# Add the current directory as an include directory.
target_include_directories(usermod_esp32camera INTERFACE
    ${CMAKE_CURRENT_LIST_DIR}
    ${IDF_PATH}/components/esp32-camera/driver/include
    ${IDF_PATH}/components/esp32-camera/driver/private_include
    ${IDF_PATH}/components/esp32-camera/conversions/include
    ${IDF_PATH}/components/esp32-camera/conversions/private_include
    ${IDF_PATH}/components/esp32-camera/sensors/private_include
)

# 尝试多个可能的JPEG头文件位置
if(EXISTS "${CMAKE_CURRENT_LIST_DIR}/../../../ports/esp32/managed_components/espressif__esp_jpeg/include")
    target_include_directories(usermod_esp32camera INTERFACE
        ${CMAKE_CURRENT_LIST_DIR}/../../../ports/esp32/managed_components/espressif__esp_jpeg/include
    )
elseif(EXISTS "${IDF_PATH}/managed_components/espressif__esp_jpeg/include")
    target_include_directories(usermod_esp32camera INTERFACE
        ${IDF_PATH}/managed_components/espressif__esp_jpeg/include
    )
endif()

target_compile_definitions(usermod_esp32camera INTERFACE)

# Link our INTERFACE library to the usermod target.
target_link_libraries(usermod INTERFACE usermod_esp32camera)
EOF

echo "✓ 相机模块配置已修复"

# 构建固件
echo "步骤4: 构建ESP32-S3相机固件..."
cd "$PROJECT_ROOT/esp/micropython/ports/esp32"

# 清理之前的构建
make clean

# 构建固件
echo "正在构建，这可能需要几分钟..."
if make USER_C_MODULES="$CAMERA_MODULE_DIR/micropython.cmake"; then
    echo "✅ 构建成功！"
    
    # 复制固件文件
    echo "步骤5: 复制固件文件..."
    BUILD_DIR="build-ESP32_GENERIC_S3"
    FIRMWARE_DIR="$PROJECT_ROOT/firmware_final_with_camera_instructions"
    
    cp "$BUILD_DIR/bootloader/bootloader.bin" "$FIRMWARE_DIR/esp32s3_camera_bootloader.bin"
    cp "$BUILD_DIR/partition_table/partition-table.bin" "$FIRMWARE_DIR/esp32s3_camera_partition_table.bin"
    cp "$BUILD_DIR/micropython.bin" "$FIRMWARE_DIR/esp32s3_camera_micropython.bin"
    cp "$BUILD_DIR/flasher_args.json" "$FIRMWARE_DIR/esp32s3_camera_flash_args.json"
    
    echo "✅ 相机固件构建完成！"
    echo ""
    echo "生成的文件:"
    echo "  - esp32s3_camera_bootloader.bin ($(stat -c%s "$FIRMWARE_DIR/esp32s3_camera_bootloader.bin") bytes)"
    echo "  - esp32s3_camera_partition_table.bin ($(stat -c%s "$FIRMWARE_DIR/esp32s3_camera_partition_table.bin") bytes)"
    echo "  - esp32s3_camera_micropython.bin ($(stat -c%s "$FIRMWARE_DIR/esp32s3_camera_micropython.bin") bytes)"
    echo ""
    echo "使用以下命令烧录:"
    echo "  cd $FIRMWARE_DIR"
    echo "  ./flash_esp32s3_camera.sh /dev/ttyUSB0"
    
else
    echo "❌ 构建失败"
    echo ""
    echo "常见解决方案:"
    echo "1. 检查ESP-IDF版本是否为v5.2"
    echo "2. 确保所有依赖包已安装"
    echo "3. 尝试清理并重新构建: make clean && make submodules"
    echo ""
    echo "如果问题持续，可以使用基础固件并通过其他方式添加相机支持"
    
    # 恢复原配置
    if [ -f "$CMAKE_FILE.backup" ]; then
        mv "$CMAKE_FILE.backup" "$CMAKE_FILE"
    fi
    
    exit 1
fi

# 恢复原配置
if [ -f "$CMAKE_FILE.backup" ]; then
    mv "$CMAKE_FILE.backup" "$CMAKE_FILE"
fi

echo ""
echo "🎉 ESP32-S3相机固件构建完成！"