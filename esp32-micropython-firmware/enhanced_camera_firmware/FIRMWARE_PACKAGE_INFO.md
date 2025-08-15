# ESP32-S3 Enhanced Camera Firmware Package
# ESP32-S3 增强相机固件包

## 包信息 (Package Information)

**版本 (Version):** v1.0.0  
**构建日期 (Build Date):** 2024年  
**目标芯片 (Target Chip):** ESP32-S3  
**MicroPython版本 (MicroPython Version):** v1.23.0  
**ESP-IDF版本 (ESP-IDF Version):** v5.2  

## 固件组件 (Firmware Components)

### 1. 启动加载器 (Bootloader)
- **文件:** `esp32s3_camera_bootloader.bin`
- **大小:** 19,456 字节 (19KB)
- **地址:** 0x0
- **SHA256:** `f2ca02bd721ddb2812ef9afb49da9e00bd94eb5f6d331d4286c0e55b8f77af75`
- **功能:** ESP32-S3 启动加载器，负责系统初始化和固件加载

### 2. 分区表 (Partition Table)
- **文件:** `esp32s3_camera_partition_table.bin`
- **大小:** 3,072 字节 (3KB)
- **地址:** 0x8000
- **SHA256:** `79d4d11acdbdad3cc82dc25b98b90c77f42eb70bf0db064bc6884e8bcc42e331`
- **功能:** 定义Flash存储器的分区布局

### 3. MicroPython固件 (MicroPython Firmware)
- **文件:** `esp32s3_camera_micropython.bin`
- **大小:** 1,677,312 字节 (1.6MB)
- **地址:** 0x10000
- **SHA256:** `3bbcfc7ec45e95afd50c732e3e1669e006c7c927bd908bbcbe6ebe59cd7b71ea`
- **功能:** MicroPython运行时环境，包含相机支持

## 增强模块 (Enhanced Module)

### camera_enhanced.py
- **类型:** Python模块
- **功能:** 动态相机引脚配置包装器
- **特性:**
  - 运行时引脚重配置
  - 多板预设支持
  - 配置保存/加载
  - 完整的错误处理
  - 与原生API兼容

## 技术规格 (Technical Specifications)

### 支持的相机接口 (Supported Camera Interfaces)
- **接口类型:** DVP (Digital Video Port)
- **数据宽度:** 8-bit
- **支持格式:** JPEG, RGB565, YUV422, GRAYSCALE
- **最大分辨率:** 1600x1200 (UXGA)
- **帧率:** 最高30fps (取决于分辨率和配置)

### GPIO引脚配置 (GPIO Pin Configuration)
支持以下相机信号引脚的动态配置：

| 信号名称 | 功能描述 | 默认引脚(ESP32-S3-CAM) |
|---------|----------|----------------------|
| XCLK    | 时钟输出 | GPIO 15 |
| SIOD    | I2C数据  | GPIO 4  |
| SIOC    | I2C时钟  | GPIO 5  |
| Y9-Y2   | 数据线   | GPIO 16-11 |
| VSYNC   | 垂直同步 | GPIO 6  |
| HREF    | 水平参考 | GPIO 7  |
| PCLK    | 像素时钟 | GPIO 13 |
| PWDN    | 电源控制 | GPIO 43 |
| RESET   | 复位信号 | GPIO 44 |

### 预设配置 (Preset Configurations)

#### ESP32-S3-CAM
```python
{
    'xclk': 15, 'siod': 4, 'sioc': 5,
    'y9': 16, 'y8': 17, 'y7': 18, 'y6': 12,
    'y5': 10, 'y4': 8, 'y3': 9, 'y2': 11,
    'vsync': 6, 'href': 7, 'pclk': 13,
    'pwdn': 43, 'reset': 44
}
```

#### ESP32-CAM (AI-Thinker)
```python
{
    'xclk': 0, 'siod': 26, 'sioc': 27,
    'y9': 35, 'y8': 34, 'y7': 39, 'y6': 36,
    'y5': 21, 'y4': 19, 'y3': 18, 'y2': 5,
    'vsync': 25, 'href': 23, 'pclk': 22,
    'pwdn': 32, 'reset': -1
}
```

#### TTGO T-Camera
```python
{
    'xclk': 32, 'siod': 13, 'sioc': 12,
    'y9': 39, 'y8': 36, 'y7': 23, 'y6': 18,
    'y5': 15, 'y4': 4, 'y3': 14, 'y2': 5,
    'vsync': 27, 'href': 25, 'pclk': 19,
    'pwdn': 26, 'reset': -1
}
```

#### M5Stack Camera
```python
{
    'xclk': 27, 'siod': 25, 'sioc': 23,
    'y9': 19, 'y8': 36, 'y7': 18, 'y6': 39,
    'y5': 5, 'y4': 34, 'y3': 35, 'y2': 32,
    'vsync': 22, 'href': 26, 'pclk': 21,
    'pwdn': -1, 'reset': 15
}
```

## API参考 (API Reference)

### 核心函数 (Core Functions)
- `init(format, framesize, quality)` - 初始化相机
- `deinit()` - 反初始化相机
- `capture()` - 拍摄照片
- `set_pins(pin_config)` - 设置引脚配置
- `get_pins()` - 获取当前引脚配置

### 预设管理 (Preset Management)
- `load_preset(board_name)` - 加载预设配置
- `list_presets()` - 列出可用预设
- `save_config(filename)` - 保存配置到文件
- `load_config(filename)` - 从文件加载配置

### 相机设置 (Camera Settings)
- `set_brightness(level)` - 设置亮度 (-2 到 2)
- `set_contrast(level)` - 设置对比度 (-2 到 2)
- `set_saturation(level)` - 设置饱和度 (-2 到 2)

### 实用功能 (Utility Functions)
- `get_status()` - 获取相机状态
- `test_camera()` - 运行相机测试

## 内存要求 (Memory Requirements)

### Flash存储器 (Flash Memory)
- **总固件大小:** ~1.7MB
- **可用用户空间:** ~2.3MB (4MB Flash)
- **建议最小Flash:** 4MB

### RAM内存 (RAM Memory)
- **系统RAM:** ~200KB (MicroPython运行时)
- **相机缓冲:** ~100KB (JPEG压缩)
- **可用用户RAM:** ~200KB

## 兼容性 (Compatibility)

### 硬件兼容性 (Hardware Compatibility)
- ✅ ESP32-S3 (所有变体)
- ✅ ESP32-S3-WROOM-1
- ✅ ESP32-S3-WROOM-2
- ✅ ESP32-S3-MINI-1

### 相机模块兼容性 (Camera Module Compatibility)
- ✅ OV2640 (推荐)
- ✅ OV3660
- ✅ OV5640
- ⚠️ 其他模块需要测试

### 开发板兼容性 (Development Board Compatibility)
- ✅ ESP32-S3-CAM
- ✅ AI-Thinker ESP32-CAM (使用适配器)
- ✅ TTGO T-Camera
- ✅ M5Stack Camera
- ✅ 自定义开发板 (通过引脚配置)

## 性能特征 (Performance Characteristics)

### 拍摄性能 (Capture Performance)
- **JPEG压缩:** 硬件加速
- **最大帧率:** 30fps @ VGA
- **最小拍摄间隔:** ~100ms
- **启动时间:** <2秒

### 功耗特征 (Power Consumption)
- **活动模式:** ~200mA @ 3.3V
- **待机模式:** ~50mA @ 3.3V
- **深度睡眠:** <1mA @ 3.3V

## 故障排除 (Troubleshooting)

### 常见问题 (Common Issues)

1. **相机初始化失败**
   - 检查引脚配置
   - 确认相机模块连接
   - 验证电源供应

2. **图像质量问题**
   - 调整亮度/对比度设置
   - 检查镜头清洁度
   - 验证时钟频率

3. **内存不足**
   - 减少图像分辨率
   - 增加JPEG压缩质量
   - 定期执行垃圾回收

### 调试工具 (Debug Tools)
- `camera.test_camera()` - 综合测试
- `camera.get_status()` - 状态检查
- `test_enhanced_camera.py` - 完整测试套件

## 更新历史 (Update History)

### v1.0.0 (2024年)
- ✨ 初始发布
- ✨ 动态引脚配置支持
- ✨ 多板预设配置
- ✨ Python包装器实现
- ✨ 完整的测试套件

## 许可证信息 (License Information)

本固件包基于以下开源项目：
- **MicroPython:** MIT License
- **ESP-IDF:** Apache License 2.0
- **Enhanced Module:** MIT License

详细许可证信息请参见各组件的相应许可证文件。

## 技术支持 (Technical Support)

如遇问题，请：
1. 查阅本文档的故障排除部分
2. 运行 `test_enhanced_camera.py` 进行诊断
3. 检查硬件连接和配置
4. 参考项目文档和示例代码

---

**注意:** 本固件为增强版本，在标准MicroPython固件基础上增加了动态相机引脚配置功能。使用前请确保硬件兼容性。