# ESP32-S3 智能摄像头 - 安装指南 🚀

## 📋 系统要求

### 硬件要求
- ESP32-S3开发板
- OV2640摄像头模块
- L298N电机驱动板（可选）
- 5V/2A电源适配器

### 软件要求
- Windows 10/11, macOS 或 Ubuntu 18.04+
- Python 3.8+
- Git

## 🔧 环境安装

### 1. 安装ESP-IDF 5.4

#### Windows用户
```powershell
# 下载ESP-IDF安装程序
https://dl.espressif.com/dl/esp-idf/

# 或使用命令行
git clone --recursive https://github.com/espressif/esp-idf.git -b v5.4
cd esp-idf
install.bat esp32s3
export.bat
```

#### macOS/Linux用户
```bash
# 安装依赖
sudo apt update && sudo apt install git python3 python3-pip python3-venv

# 克隆ESP-IDF
git clone --recursive https://github.com/espressif/esp-idf.git -b v5.4
cd esp-idf

# 安装工具链
./install.sh esp32s3

# 设置环境变量
source export.sh
```

### 2. 验证安装
```bash
idf.py --version
# 应该显示: ESP-IDF v5.4.x
```

## 📦 项目部署

### 1. 获取项目代码
```bash
git clone <项目地址>
cd esp32s3-camera-ai-project
```

### 2. 配置项目
```bash
# 设置目标芯片
idf.py set-target esp32s3

# 可选：打开配置菜单
idf.py menuconfig
```

### 3. 编译项目
```bash
idf.py build
```

### 4. 连接硬件
- 使用USB-C线连接ESP32-S3到电脑
- 按住BOOT按钮，然后按RESET按钮进入下载模式

### 5. 烧录固件
```bash
# Windows (COM端口)
idf.py -p COM3 flash

# macOS/Linux (USB端口)
idf.py -p /dev/ttyUSB0 flash

# 监控串口输出
idf.py -p <PORT> monitor
```

## 🔌 硬件连接

### 摄像头连接（OV2640）
| ESP32-S3 | OV2640 |
|----------|--------|
| GPIO4    | SDA    |
| GPIO5    | SCL    |
| GPIO6    | VSYNC  |
| GPIO7    | HREF   |
| GPIO15   | PCLK   |
| GPIO16   | XCLK   |
| GPIO17   | D7     |
| GPIO18   | D6     |
| GPIO8    | D5     |
| GPIO9    | D4     |
| GPIO10   | D3     |
| GPIO11   | D2     |
| GPIO12   | D1     |
| GPIO13   | D0     |
| GPIO14   | RESET  |
| 3.3V     | VCC    |
| GND      | GND    |

### 电机驱动连接（L298N）
| ESP32-S3 | L298N |
|----------|-------|
| GPIO1    | IN1   |
| GPIO2    | IN2   |
| GPIO42   | IN3   |
| GPIO41   | IN4   |
| 5V       | VCC   |
| GND      | GND   |

## 🌐 首次使用

### 1. 连接WiFi
设备启动后会创建热点：
- **热点名称**: `ESP32-S3-Camera`
- **密码**: `12345678`
- **访问地址**: `http://192.168.4.1`

### 2. 配置网络
在Web界面中输入您的WiFi信息，设备会自动连接。

### 3. 功能测试
- 📹 **视频流**: 查看实时画面
- 🎮 **电机控制**: 测试运动功能
- 🤖 **AI识别**: 尝试物体检测

## ⚠️ 常见问题

### 烧录失败
```bash
# 检查端口
ls /dev/tty*  # Linux/macOS
# 或在设备管理器中查看 # Windows

# 尝试不同波特率
idf.py -p <PORT> -b 115200 flash
```

### 编译错误
```bash
# 清理重新编译
rm -rf build
idf.py build
```

### 摄像头无图像
- 检查接线是否正确
- 确认电源供应充足（5V/2A）
- 查看串口输出错误信息

### WiFi连接问题
- 确认使用2.4GHz网络
- 检查密码是否正确
- 查看串口日志获取详细信息

## 📞 技术支持

遇到问题？
1. 查看串口输出日志
2. 检查硬件连接
3. 参考完整README文档
4. 提交GitHub Issue

---

**🎉 安装完成！开始享受您的智能摄像头吧！**