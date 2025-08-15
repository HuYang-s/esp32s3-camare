#!/bin/bash

# Enhanced ESP32-S3相机固件构建脚本
# 此脚本将构建包含动态引脚配置功能的ESP32-S3 MicroPython相机固件

set -e

echo "Enhanced ESP32-S3相机固件构建脚本"
echo "================================="
echo "支持动态引脚配置的相机模块"
echo ""

# 检查是否在正确的目录
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"

if [ ! -d "$PROJECT_ROOT/esp/esp-idf" ]; then
    echo "错误: ESP-IDF未找到，请确保在正确的项目目录中运行此脚本"
    echo "提示: 可能需要先运行setup脚本设置环境"
    exit 1
fi

echo "项目根目录: $PROJECT_ROOT"
echo "增强模块目录: $SCRIPT_DIR"
echo ""

# 激活ESP-IDF环境
echo "步骤1: 激活ESP-IDF环境..."
cd "$PROJECT_ROOT"
source esp/esp-idf/export.sh

# 检查MicroPython源码
echo "步骤2: 检查MicroPython源码..."
MICROPYTHON_DIR="$PROJECT_ROOT/esp/micropython"
if [ ! -d "$MICROPYTHON_DIR" ]; then
    echo "错误: MicroPython源码未找到，请先运行setup脚本"
    exit 1
fi

echo "✓ MicroPython源码已准备就绪"

# 创建用户模块目录（如果不存在）
echo "步骤3: 准备用户模块目录..."
USERMOD_DIR="$MICROPYTHON_DIR/examples/usercmodule/camera_enhanced"
mkdir -p "$USERMOD_DIR"

# 复制增强的相机模块文件
echo "步骤4: 复制增强相机模块..."
cp "$SCRIPT_DIR/modcamera_enhanced.c" "$USERMOD_DIR/"
cp "$SCRIPT_DIR/micropython.cmake" "$USERMOD_DIR/"

echo "✓ 增强相机模块已复制到用户模块目录"

# 检查ESP32-camera组件
echo "步骤5: 检查ESP32-camera组件..."
ESP32_PORT_DIR="$MICROPYTHON_DIR/ports/esp32"
cd "$ESP32_PORT_DIR"

# 确保ESP32-camera组件存在
if [ ! -f "managed_components/espressif__esp32-camera/idf_component.yml" ]; then
    echo "添加ESP32-camera组件依赖..."
    
    # 创建或更新main/idf_component.yml
    mkdir -p main
    cat > main/idf_component.yml << EOF
## IDF Component Manager Manifest File
dependencies:
  espressif/esp32-camera: ">=2.0.0"
  espressif/esp_jpeg: "*"
EOF
    
    echo "✓ ESP32-camera组件依赖已配置"
fi

# 构建固件
echo "步骤6: 构建增强ESP32-S3相机固件..."

# 清理之前的构建
make clean

# 设置构建目标为ESP32-S3
export BOARD=ESP32_GENERIC_S3

# 构建固件（使用增强的相机模块）
echo "正在构建增强固件，这可能需要几分钟..."
if make USER_C_MODULES="$USERMOD_DIR/micropython.cmake" BOARD=ESP32_GENERIC_S3; then
    echo "✅ 增强固件构建成功！"
    
    # 复制固件文件到增强模块目录
    echo "步骤7: 复制固件文件..."
    BUILD_DIR="build-ESP32_GENERIC_S3"
    
    cp "$BUILD_DIR/bootloader/bootloader.bin" "$SCRIPT_DIR/esp32s3_enhanced_camera_bootloader.bin"
    cp "$BUILD_DIR/partition_table/partition-table.bin" "$SCRIPT_DIR/esp32s3_enhanced_camera_partition_table.bin"
    cp "$BUILD_DIR/micropython.bin" "$SCRIPT_DIR/esp32s3_enhanced_camera_micropython.bin"
    cp "$BUILD_DIR/flasher_args.json" "$SCRIPT_DIR/esp32s3_enhanced_camera_flash_args.json"
    
    echo "✅ 增强相机固件构建完成！"
    echo ""
    echo "生成的文件:"
    echo "  - esp32s3_enhanced_camera_bootloader.bin ($(stat -c%s "$SCRIPT_DIR/esp32s3_enhanced_camera_bootloader.bin") bytes)"
    echo "  - esp32s3_enhanced_camera_partition_table.bin ($(stat -c%s "$SCRIPT_DIR/esp32s3_enhanced_camera_partition_table.bin") bytes)"
    echo "  - esp32s3_enhanced_camera_micropython.bin ($(stat -c%s "$SCRIPT_DIR/esp32s3_enhanced_camera_micropython.bin") bytes)"
    echo ""
    echo "新功能特性:"
    echo "  ✓ 动态引脚配置支持"
    echo "  ✓ 改进的错误处理"
    echo "  ✓ 更灵活的相机初始化"
    echo "  ✓ 引脚状态查询功能"
    echo ""
    echo "使用以下命令烧录:"
    echo "  cd $SCRIPT_DIR"
    echo "  ./flash_enhanced_camera.sh /dev/ttyUSB0"
    echo ""
    echo "示例代码:"
    echo "  import camera"
    echo "  # 使用自定义引脚配置"
    echo "  pins = {'xclk': 0, 'sda': 26, 'scl': 27, 'd7': 35, 'd6': 34, ...}"
    echo "  camera.set_pins(pins)"
    echo "  camera.init()"
    
else
    echo "❌ 构建失败"
    echo ""
    echo "常见解决方案:"
    echo "1. 检查ESP-IDF版本是否为v5.0+"
    echo "2. 确保所有依赖包已安装"
    echo "3. 尝试清理并重新构建: make clean && make submodules"
    echo "4. 检查ESP32-camera组件是否正确安装"
    echo ""
    
    exit 1
fi

echo ""
echo "🎉 增强ESP32-S3相机固件构建完成！"
echo ""
echo "下一步:"
echo "1. 烧录固件到ESP32-S3设备"
echo "2. 查看使用文档了解新的API功能"
echo "3. 运行测试脚本验证功能"