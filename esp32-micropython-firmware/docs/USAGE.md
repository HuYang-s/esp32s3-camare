# ESP32-S3 MicroPython固件构建详细说明

## 快速开始

### 方法1: 一键构建（推荐）
```bash
./scripts/build-all.sh
```

### 方法2: 分步构建
```bash
# 1. 设置环境
./scripts/setup.sh

# 2. 激活ESP-IDF环境（每次新会话都需要）
source esp/esp-idf/export.sh

# 3. 构建固件
./scripts/build.sh
```

## 系统要求

- Ubuntu 20.04 或更高版本
- 至少 8GB 可用磁盘空间
- 稳定的网络连接（用于下载依赖）

## 构建过程说明

### 环境设置阶段（setup.sh）
1. 安装Ubuntu依赖包
2. 配置pip源到阿里云镜像
3. 克隆esp-idf、micropython等仓库
4. 切换ESP-IDF到v5.0.2版本
5. 配置ESP-IDF开发环境

### 固件构建阶段（build.sh）
1. 构建基础ESP32 MicroPython固件
2. 下载并配置相机驱动
3. 修改配置以支持ESP32-S3
4. 构建ESP32-S3相机功能固件

## 生成的固件文件

构建完成后，在`firmware/`目录下会生成以下文件：

- `esp32-generic-firmware.bin` - 基础ESP32 MicroPython固件
- `esp32-s3-camera-firmware.bin` - ESP32-S3相机功能MicroPython固件

## 固件烧录

使用esptool.py烧录固件到ESP32-S3开发板：

```bash
# 激活ESP-IDF环境
source esp/esp-idf/export.sh

# 烧录ESP32-S3相机固件
esptool.py --chip esp32s3 --port /dev/ttyUSB0 --baud 460800 write_flash -z 0x0 firmware/esp32-s3-camera-firmware.bin

# 或者使用idf.py（如果有项目配置）
idf.py -p /dev/ttyUSB0 flash
```

## 相机功能测试

烧录固件后，可以通过MicroPython REPL测试相机功能：

```python
import camera

# 初始化相机
camera.init()

# 拍照并获取图像数据
img = camera.capture()

# 释放相机资源
camera.deinit()
```

## 故障排除

### 1. 依赖包安装失败
```bash
# 更新包管理器并重试
sudo apt-get update
sudo apt-get install --fix-missing [package-name]
```

### 2. 子模块更新失败
```bash
# 手动更新子模块
cd esp/esp-idf
git submodule update --init --recursive --force
```

### 3. 编译失败
```bash
# 清理并重新构建
cd esp/micropython/ports/esp32
make clean
make
```

### 4. 相机模块导入失败
确保：
- 使用的是ESP32-S3固件
- 相机模块已正确编译到固件中
- 硬件连接正确

## 自定义配置

### 修改目标开发板
编辑`esp/micropython/ports/esp32/Makefile`文件，修改BOARD变量：
```makefile
BOARD ?= ESP32_GENERIC_S3  # 可选: ESP32_GENERIC, ESP32_GENERIC_S3等
```

### 添加其他模块
参考MicroPython官方文档，在`examples/usercmodule/`目录下添加自定义C模块。

## 参考资料

- [ESP-IDF官方文档](https://docs.espressif.com/projects/esp-idf/)
- [MicroPython官方文档](https://docs.micropython.org/)
- [ESP32相机驱动文档](https://github.com/espressif/esp32-camera)