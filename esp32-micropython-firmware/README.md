# ESP32-S3 MicroPython 固件构建项目

本项目用于构建ESP32-S3的MicroPython固件，包含相机驱动支持。基于ESP-IDF v5.0.2和最新的MicroPython源码。

## 🚀 快速开始

### 一键构建（推荐）
```bash
git clone <this-repo>
cd esp32-micropython-firmware
./scripts/build-all.sh
```

### 分步构建
```bash
# 1. 设置环境（仅需运行一次）
./scripts/setup.sh

# 2. 激活ESP-IDF环境（每次新会话都需要）
source esp/esp-idf/export.sh

# 3. 构建固件
./scripts/build.sh
```

## 📁 项目结构
```
esp32-micropython-firmware/
├── scripts/           # 构建脚本
│   ├── setup.sh      # 环境设置脚本
│   ├── build.sh      # 固件构建脚本
│   └── build-all.sh  # 一键构建脚本
├── esp/              # ESP-IDF和MicroPython源码（自动生成）
├── firmware/         # 生成的固件文件
├── docs/             # 详细文档
│   └── USAGE.md      # 使用说明
└── README.md         # 本文件
```

## 🎯 生成的固件

构建完成后，在`firmware/`目录下会生成：

- **esp32-generic-firmware.bin** - 基础ESP32 MicroPython固件
- **esp32-s3-camera-firmware.bin** - ESP32-S3相机功能MicroPython固件 ⭐

## 📋 系统要求

- Ubuntu 20.04 或更高版本
- 至少 8GB 可用磁盘空间
- 稳定的网络连接

## 🔧 主要特性

- ✅ 基于ESP-IDF v5.0.2
- ✅ 支持ESP32和ESP32-S3
- ✅ 集成相机驱动（ESP32-S3）
- ✅ 自动化构建脚本
- ✅ 中文镜像源加速
- ✅ 详细的错误处理

## 📖 详细文档

更多详细信息请查看：
- [详细使用说明](docs/USAGE.md)

## 🔥 相机功能测试

烧录ESP32-S3固件后，可以通过MicroPython REPL测试：

```python
import camera

# 初始化相机
camera.init()

# 拍照
img = camera.capture()
print(f"图像大小: {len(img)} bytes")

# 释放资源
camera.deinit()
```

## ⚡ 故障排除

如果遇到问题，请查看[详细使用说明](docs/USAGE.md)中的故障排除部分。

## 🤝 贡献

欢迎提交Issue和Pull Request！

## 📄 许可证

本项目遵循相关开源项目的许可证：
- ESP-IDF: Apache License 2.0
- MicroPython: MIT License