"""
OV2640 相机模块专用配置和使用示例
OV2640 Camera Module Specific Configuration and Usage Example

GPIO配置:
PWDN_GPIO_NUM  = -1 (未使用)
RESET_GPIO_NUM = -1 (未使用)  
XCLK_GPIO_NUM  = 15
SIOD_GPIO_NUM  = 4
SIOC_GPIO_NUM  = 5
Y2_GPIO_NUM    = 11
Y3_GPIO_NUM    = 9
Y4_GPIO_NUM    = 8
Y5_GPIO_NUM    = 10
Y6_GPIO_NUM    = 12
Y7_GPIO_NUM    = 18
Y8_GPIO_NUM    = 17
Y9_GPIO_NUM    = 16
VSYNC_GPIO_NUM = 6
HREF_GPIO_NUM  = 7
PCLK_GPIO_NUM  = 13
"""

import camera_enhanced as camera
import time
import os

def setup_ov2640_pins():
    """配置OV2640相机引脚"""
    print("=== 配置OV2640相机引脚 ===")
    
    # OV2640专用引脚配置
    ov2640_pins = {
        # 控制引脚
        'pwdn': -1,      # 电源控制引脚 (未使用)
        'reset': -1,     # 复位引脚 (未使用)
        'xclk': 15,      # 时钟输出引脚
        
        # I2C引脚 (SCCB接口)
        'sda': 4,        # SIOD - I2C数据线
        'scl': 5,        # SIOC - I2C时钟线
        
        # 8位并行数据引脚 (DVP接口)
        'd0': 11,        # Y2 - 数据位0
        'd1': 9,         # Y3 - 数据位1  
        'd2': 8,         # Y4 - 数据位2
        'd3': 10,        # Y5 - 数据位3
        'd4': 12,        # Y6 - 数据位4
        'd5': 18,        # Y7 - 数据位5
        'd6': 17,        # Y8 - 数据位6
        'd7': 16,        # Y9 - 数据位7
        
        # 同步信号引脚
        'vsync': 6,      # 垂直同步信号
        'href': 7,       # 水平参考信号  
        'pclk': 13,      # 像素时钟信号
    }
    
    print("设置OV2640引脚配置...")
    camera.set_pins(ov2640_pins)
    
    # 显示配置信息
    print("OV2640引脚配置:")
    print("控制引脚:")
    print(f"  PWDN:  GPIO{ov2640_pins['pwdn']} {'(未使用)' if ov2640_pins['pwdn'] == -1 else ''}")
    print(f"  RESET: GPIO{ov2640_pins['reset']} {'(未使用)' if ov2640_pins['reset'] == -1 else ''}")
    print(f"  XCLK:  GPIO{ov2640_pins['xclk']}")
    
    print("I2C接口 (SCCB):")
    print(f"  SIOD:  GPIO{ov2640_pins['sda']}")
    print(f"  SIOC:  GPIO{ov2640_pins['scl']}")
    
    print("数据引脚 (DVP):")
    for i in range(8):
        pin_name = f'd{i}'
        y_name = f'Y{i+2}'
        print(f"  {y_name}:    GPIO{ov2640_pins[pin_name]}")
    
    print("同步信号:")
    print(f"  VSYNC: GPIO{ov2640_pins['vsync']}")
    print(f"  HREF:  GPIO{ov2640_pins['href']}")
    print(f"  PCLK:  GPIO{ov2640_pins['pclk']}")
    
    return ov2640_pins

def test_ov2640_resolutions():
    """测试OV2640支持的不同分辨率"""
    print("\n=== 测试OV2640分辨率 ===")
    
    # OV2640支持的分辨率
    resolutions = [
        (camera.FRAME_QQVGA, "QQVGA", "160x120"),
        (camera.FRAME_QVGA, "QVGA", "320x240"), 
        (camera.FRAME_VGA, "VGA", "640x480"),
        (camera.FRAME_SVGA, "SVGA", "800x600"),
        (camera.FRAME_XGA, "XGA", "1024x768"),
        (camera.FRAME_SXGA, "SXGA", "1280x1024"),
        (camera.FRAME_UXGA, "UXGA", "1600x1200"),
    ]
    
    successful_resolutions = []
    
    for frame_size, name, resolution in resolutions:
        print(f"\n测试 {name} ({resolution})...")
        
        try:
            # 重新初始化相机
            camera.deinit()
            time.sleep(1)
            
            camera.init(
                format=camera.JPEG,
                framesize=frame_size,
                quality=12
            )
            
            time.sleep(2)  # 等待稳定
            
            # 尝试拍摄
            image_data = camera.capture()
            
            if image_data and len(image_data) > 0:
                print(f"✓ {name} 成功! 图像大小: {len(image_data)} 字节")
                successful_resolutions.append((name, resolution, len(image_data)))
                
                # 保存测试图像
                filename = f"test_{name.lower()}_{int(time.time())}.jpg"
                with open(filename, 'wb') as f:
                    f.write(image_data)
                print(f"  保存为: {filename}")
            else:
                print(f"✗ {name} 失败: 无图像数据")
                
        except Exception as e:
            print(f"✗ {name} 失败: {e}")
    
    # 总结
    print(f"\n=== 分辨率测试总结 ===")
    print(f"成功: {len(successful_resolutions)}/{len(resolutions)}")
    
    if successful_resolutions:
        print("支持的分辨率:")
        for name, resolution, size in successful_resolutions:
            print(f"  ✓ {name} ({resolution}) - 平均大小: {size} 字节")
    
    return successful_resolutions

def optimize_ov2640_settings():
    """优化OV2640相机设置"""
    print("\n=== 优化OV2640设置 ===")
    
    # 重新初始化为最佳设置
    camera.deinit()
    time.sleep(1)
    
    try:
        # 使用VGA分辨率，这对OV2640来说是个很好的平衡点
        camera.init(
            format=camera.JPEG,
            framesize=camera.FRAME_VGA,  # 640x480
            quality=10  # 高质量
        )
        
        print("✓ 相机初始化完成 (VGA, 质量=10)")
        time.sleep(2)
        
        # 优化图像设置
        print("调整图像参数...")
        
        # 轻微增加对比度，适合大多数场景
        camera.set_contrast(1)
        print("  对比度: +1")
        
        # 保持默认亮度
        camera.set_brightness(0)
        print("  亮度: 0 (默认)")
        
        # 轻微增加饱和度，让颜色更鲜艳
        camera.set_saturation(1)
        print("  饱和度: +1")
        
        print("✓ OV2640设置优化完成")
        
        return True
        
    except Exception as e:
        print(f"✗ 设置优化失败: {e}")
        return False

def capture_ov2640_photos():
    """使用优化设置拍摄OV2640照片"""
    print("\n=== OV2640拍摄测试 ===")
    
    photos_taken = 0
    
    try:
        # 拍摄测试照片
        for i in range(3):
            print(f"\n拍摄第 {i+1} 张照片...")
            
            # 拍摄前稍等，让自动曝光稳定
            time.sleep(1)
            
            image_data = camera.capture()
            
            if image_data and len(image_data) > 0:
                timestamp = int(time.time())
                filename = f"ov2640_photo_{i+1}_{timestamp}.jpg"
                
                with open(filename, 'wb') as f:
                    f.write(image_data)
                
                print(f"✓ 拍摄成功! 文件: {filename}")
                print(f"  图像大小: {len(image_data)} 字节")
                photos_taken += 1
            else:
                print(f"✗ 第 {i+1} 张拍摄失败")
            
            if i < 2:  # 不是最后一张
                print("等待 3 秒...")
                time.sleep(3)
        
        print(f"\n✓ 完成拍摄! 成功: {photos_taken}/3 张")
        
    except Exception as e:
        print(f"✗ 拍摄过程出错: {e}")
    
    return photos_taken

def list_ov2640_photos():
    """列出OV2640拍摄的照片"""
    print("\n=== OV2640照片列表 ===")
    
    try:
        files = os.listdir('.')
        ov2640_photos = [f for f in files if f.startswith('ov2640_') and f.endswith('.jpg')]
        test_photos = [f for f in files if f.startswith('test_') and f.endswith('.jpg')]
        
        all_photos = ov2640_photos + test_photos
        
        if all_photos:
            print(f"找到 {len(all_photos)} 张OV2640照片:")
            
            total_size = 0
            for i, filename in enumerate(all_photos, 1):
                try:
                    stat = os.stat(filename)
                    size = stat[6]
                    total_size += size
                    print(f"  {i:2d}. {filename} ({size:,} 字节)")
                except:
                    print(f"  {i:2d}. {filename} (大小未知)")
            
            print(f"\n总计: {len(all_photos)} 张照片, {total_size:,} 字节")
        else:
            print("没有找到OV2640照片")
            
    except Exception as e:
        print(f"列出照片时出错: {e}")

def main():
    """OV2640相机完整测试流程"""
    print("OV2640 相机模块完整测试")
    print("=" * 50)
    
    try:
        # 1. 配置引脚
        pins = setup_ov2640_pins()
        
        # 2. 检查相机状态
        print(f"\n当前相机状态:")
        status = camera.get_status()
        for key, value in status.items():
            print(f"  {key}: {value}")
        
        # 3. 测试不同分辨率 (可选)
        test_resolutions = input("\n是否要测试所有分辨率? (y/n): ")
        if test_resolutions.lower() == 'y':
            test_ov2640_resolutions()
        
        # 4. 优化设置
        if optimize_ov2640_settings():
            # 5. 拍摄照片
            capture_ov2640_photos()
        
        # 6. 列出照片
        list_ov2640_photos()
        
        print("\n" + "=" * 50)
        print("✓ OV2640测试完成!")
        print("\n使用提示:")
        print("- OV2640最佳分辨率: VGA (640x480)")
        print("- 建议质量设置: 8-12")
        print("- 支持最大分辨率: UXGA (1600x1200)")
        print("- 拍摄间隔: 至少1秒让自动曝光稳定")
        
    except KeyboardInterrupt:
        print("\n用户中断测试")
    except Exception as e:
        print(f"\n测试过程出错: {e}")
        import sys
        sys.print_exception(e)
    finally:
        # 清理资源
        try:
            camera.deinit()
            print("\n相机资源已清理")
        except:
            pass

if __name__ == "__main__":
    main()