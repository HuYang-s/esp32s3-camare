"""
OV2640相机快速配置和拍摄
OV2640 Camera Quick Setup and Capture
"""

import camera_enhanced as camera
import time

print("OV2640 相机快速设置")
print("=" * 30)

# 1. 设置OV2640专用引脚
print("配置OV2640引脚...")
ov2640_pins = {
    'pwdn': -1,    # 未使用
    'reset': -1,   # 未使用  
    'xclk': 15,    # 时钟
    'sda': 4,      # SIOD
    'scl': 5,      # SIOC
    'd0': 11,      # Y2
    'd1': 9,       # Y3
    'd2': 8,       # Y4
    'd3': 10,      # Y5
    'd4': 12,      # Y6
    'd5': 18,      # Y7
    'd6': 17,      # Y8
    'd7': 16,      # Y9
    'vsync': 6,    # VSYNC
    'href': 7,     # HREF
    'pclk': 13,    # PCLK
}

camera.set_pins(ov2640_pins)
print("✓ OV2640引脚配置完成")

# 2. 初始化相机
print("初始化OV2640相机...")
camera.init(
    format=camera.JPEG,
    framesize=camera.FRAME_VGA,  # 640x480
    quality=10
)

# 3. 等待稳定
print("等待相机稳定...")
time.sleep(3)

# 4. 优化设置
print("优化相机设置...")
camera.set_brightness(0)   # 默认亮度
camera.set_contrast(1)     # 稍微增加对比度
camera.set_saturation(1)   # 稍微增加饱和度

# 5. 拍摄照片
print("拍摄照片...")
image_data = camera.capture()

if image_data:
    filename = f"ov2640_{int(time.time())}.jpg"
    with open(filename, 'wb') as f:
        f.write(image_data)
    
    print(f"✓ 拍摄成功!")
    print(f"文件: {filename}")
    print(f"大小: {len(image_data)} 字节")
else:
    print("✗ 拍摄失败")

# 6. 清理
camera.deinit()
print("完成!")