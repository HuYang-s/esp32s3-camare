"""
ESP32-S3 相机快速使用示例
Quick Camera Usage Example
"""

import camera_enhanced as camera
import time

# === 快速开始 ===
print("ESP32-S3 相机快速示例")

# 1. 设置引脚配置（使用预设）
print("设置相机引脚...")
camera.load_preset('ESP32-S3-CAM')  # 或 'AI-Thinker' 等其他预设

# 2. 初始化相机
print("初始化相机...")
camera.init(
    format=camera.JPEG,         # JPEG格式
    framesize=camera.FRAME_VGA, # VGA分辨率 (640x480)
    quality=10                  # 质量设置 (1-63, 越小质量越好)
)

# 等待相机稳定
time.sleep(2)

# 3. 拍摄照片
print("拍摄照片...")
image_data = camera.capture()

if image_data:
    print(f"拍摄成功! 图像大小: {len(image_data)} 字节")
    
    # 4. 保存照片
    filename = f"photo_{time.ticks_ms()}.jpg"
    with open(filename, 'wb') as f:
        f.write(image_data)
    
    print(f"照片已保存为: {filename}")
else:
    print("拍摄失败")

# 5. 清理资源
camera.deinit()
print("完成!")