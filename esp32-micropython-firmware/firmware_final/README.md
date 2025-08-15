# ESP32/ESP32-S3 MicroPython 固件包

## 📦 固件包内容

本目录包含完整的ESP32和ESP32-S3 MicroPython固件，所有文件已构建完成并可直接使用。

### ESP32固件文件
- `esp32_bootloader.bin` - ESP32引导加载程序 (23KB)
- `esp32_partition_table.bin` - ESP32分区表 (3KB)
- `esp32_micropython.bin` - ESP32 MicroPython固件 (1.6MB)
- `esp32_flash_args.json` - ESP32烧录参数
- `flash_esp32.sh` - ESP32烧录脚本

### ESP32-S3固件文件
- `esp32s3_bootloader.bin` - ESP32-S3引导加载程序 (19KB)
- `esp32s3_partition_table.bin` - ESP32-S3分区表 (3KB)
- `esp32s3_micropython.bin` - ESP32-S3 MicroPython固件 (1.5MB)
- `esp32s3_flash_args.json` - ESP32-S3烧录参数
- `flash_esp32s3.sh` - ESP32-S3烧录脚本

## 🚀 快速烧录

### ESP32烧录
```bash
./flash_esp32.sh /dev/ttyUSB0 460800
```

### ESP32-S3烧录
```bash
./flash_esp32s3.sh /dev/ttyUSB0 460800
```

## 📋 手动烧录

如果需要手动烧录，使用以下命令：

### ESP32手动烧录
```bash
esptool.py --chip esp32 --port /dev/ttyUSB0 --baud 460800 \
    --before default_reset --after hard_reset \
    write_flash --flash_mode dio --flash_freq 40m --flash_size 4MB \
    0x1000 esp32_bootloader.bin \
    0x8000 esp32_partition_table.bin \
    0x10000 esp32_micropython.bin
```

### ESP32-S3手动烧录
```bash
esptool.py --chip esp32s3 --port /dev/ttyUSB0 --baud 460800 \
    --before default_reset --after hard_reset \
    write_flash --flash_mode dio --flash_freq 80m --flash_size 8MB \
    0x0 esp32s3_bootloader.bin \
    0x8000 esp32s3_partition_table.bin \
    0x10000 esp32s3_micropython.bin
```

## 📊 固件信息

| 项目 | ESP32 | ESP32-S3 |
|------|-------|----------|
| MicroPython版本 | v1.24.1 | v1.24.1 |
| ESP-IDF版本 | v5.2 | v5.2 |
| 总固件大小 | ~1.7MB | ~1.6MB |
| Flash大小 | 4MB | 8MB |
| 构建时间 | $(date) | $(date) |

## ⚡ 烧录后验证

烧录完成后，连接串口查看启动信息：

```bash
screen /dev/ttyUSB0 115200
```

正常启动应显示：
```
MicroPython v1.24.1 on 2024-XX-XX; Generic ESP32 module with ESP32
Type "help()" for more information.
>>>
```

或者对于ESP32-S3：
```
MicroPython v1.24.1 on 2024-XX-XX; Generic ESP32S3 module with ESP32S3
Type "help()" for more information.
>>>
```

## 🔧 故障排除

### 常见问题

1. **串口权限问题**
   ```bash
   sudo usermod -a -G dialout $USER
   # 然后重新登录
   ```

2. **ESP32-S3启动失败**
   - 确保使用正确的地址：ESP32-S3的bootloader在0x0，不是0x1000
   - 检查是否烧录了所有三个文件

3. **esptool.py未找到**
   ```bash
   pip install esptool
   ```

### 擦除Flash（如果需要）
```bash
# ESP32
esptool.py --chip esp32 --port /dev/ttyUSB0 erase_flash

# ESP32-S3
esptool.py --chip esp32s3 --port /dev/ttyUSB0 erase_flash
```

## 📝 技术规格

- **编译器**: xtensa-esp-elf-gcc 13.2.0
- **构建系统**: ESP-IDF v5.2 + CMake + Ninja
- **Python环境**: Python 3.13.3
- **支持功能**: WiFi, Bluetooth, 文件系统, 网络协议栈

## 🎯 使用建议

1. **首次烧录**: 建议先擦除整个Flash，然后烧录固件
2. **开发调试**: 可以使用 `idf.py monitor` 进行串口监控
3. **固件更新**: 通常只需要更新application部分(micropython.bin)
4. **备份**: 建议备份原厂固件以便恢复

---
*固件构建于: $(date)*
*ESP-IDF版本: v5.2*
*MicroPython版本: v1.24.1*