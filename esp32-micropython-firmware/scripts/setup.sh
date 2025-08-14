#!/bin/bash

set -e

echo "开始设置ESP32-S3 MicroPython固件构建环境..."

# 获取脚本所在目录的绝对路径
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"
ESP_DIR="$PROJECT_ROOT/esp"

echo "项目根目录: $PROJECT_ROOT"
echo "ESP目录: $ESP_DIR"

# 01. 安装Ubuntu依赖包
echo "步骤1: 安装Ubuntu依赖包..."
sudo apt-get update
sudo apt-get install -y git wget libncurses-dev flex bison gperf python3 python3-pip python3-setuptools python3-serial python3-click python3-cryptography python3-future python3-pyparsing python3-pyelftools cmake ninja-build ccache libffi-dev libssl-dev python-is-python3 python3.10-venv --fix-missing

# 02. 配置pip源
echo "步骤2: 配置pip源到阿里云..."
pip config set global.index-url http://mirrors.aliyun.com/pypi/simple
pip config set global.trusted-host mirrors.aliyun.com

# 03. 克隆仓库
echo "步骤3: 克隆esp-idf和micropython仓库..."
mkdir -p "$ESP_DIR"
cd "$ESP_DIR"

if [ ! -d "esp-gitee-tools" ]; then
    git clone https://gitee.com/EspressifSystems/esp-gitee-tools.git
fi

if [ ! -d "esp-idf" ]; then
    git clone https://gitee.com/EspressifSystems/esp-idf.git
fi

if [ ! -d "micropython" ]; then
    git clone https://github.com/micropython/micropython.git
fi

# 04. 切换ESP-IDF版本到v5.0.2
echo "步骤4: 切换ESP-IDF版本到v5.0.2..."
cd "$ESP_DIR/esp-idf"
git checkout v5.0.2

# 05. 配置esp-idf
echo "步骤5: 配置esp-idf环境..."
# 使用jihu镜像更新子模块
cd "$ESP_DIR/esp-gitee-tools"
./jihu-mirror.sh set
./submodule-update.sh "$ESP_DIR/esp-idf/"
./install.sh "$ESP_DIR/esp-idf/"

echo "环境设置完成！"
echo "请运行以下命令激活环境变量："
echo "source $ESP_DIR/esp-idf/export.sh"
echo ""
echo "然后运行构建脚本："
echo "./scripts/build.sh"