"""
ESP32-S3 相机引脚配置、拍摄和保存完整示例
Complete example for camera pin configuration, capture and save
"""

import camera_enhanced as camera
import time
import os

def setup_camera_pins():
    """设置相机引脚配置"""
    print("=== 相机引脚配置 ===")
    
    # 方法1: 使用预设配置（推荐）
    print("1. 使用预设配置...")
    available_presets = camera.list_presets()
    print(f"可用预设: {available_presets}")
    
    # 为ESP32-S3-CAM使用预设
    camera.load_preset('ESP32-S3-CAM')
    print("✓ 已加载ESP32-S3-CAM预设配置")
    
    # 方法2: 自定义引脚配置
    print("\n2. 或者使用自定义配置...")
    custom_pins = {
        # 控制引脚
        'pwdn': 43,      # 电源控制引脚 (Power Down)
        'reset': 44,     # 复位引脚 (Reset)
        'xclk': 15,      # 时钟输出引脚 (External Clock)
        
        # I2C引脚 (用于相机配置)
        'sda': 4,        # I2C数据引脚 (SIOD)
        'scl': 5,        # I2C时钟引脚 (SIOC)
        
        # 数据引脚 (8位并行数据)
        'd7': 16,        # 数据位7 (MSB)
        'd6': 17,        # 数据位6
        'd5': 18,        # 数据位5
        'd4': 12,        # 数据位4
        'd3': 10,        # 数据位3
        'd2': 8,         # 数据位2
        'd1': 9,         # 数据位1
        'd0': 11,        # 数据位0 (LSB)
        
        # 同步引脚
        'vsync': 6,      # 垂直同步 (Vertical Sync)
        'href': 7,       # 水平参考 (Horizontal Reference)
        'pclk': 13,      # 像素时钟 (Pixel Clock)
    }
    
    # 如果需要自定义配置，取消下面的注释
    # camera.set_pins(custom_pins)
    # print("✓ 已设置自定义引脚配置")
    
    # 显示当前配置
    current_pins = camera.get_pins()
    print(f"\n当前引脚配置:")
    for pin_name, pin_num in current_pins.items():
        if pin_num >= 0:
            print(f"  {pin_name}: GPIO{pin_num}")
        else:
            print(f"  {pin_name}: 未使用")

def initialize_camera():
    """初始化相机"""
    print("\n=== 初始化相机 ===")
    
    try:
        # 初始化相机，设置参数
        camera.init(
            format=camera.JPEG,           # 图像格式: JPEG
            framesize=camera.FRAME_VGA,   # 分辨率: VGA (640x480)
            quality=10                    # JPEG质量: 10 (1-63, 数字越小质量越好)
        )
        print("✓ 相机初始化成功")
        
        # 等待相机稳定
        print("等待相机稳定...")
        time.sleep(2)
        
        return True
        
    except Exception as e:
        print(f"✗ 相机初始化失败: {e}")
        return False

def adjust_camera_settings():
    """调整相机设置"""
    print("\n=== 调整相机设置 ===")
    
    try:
        # 设置亮度 (-2到2, 0为默认)
        camera.set_brightness(0)
        print("✓ 亮度设置为: 0")
        
        # 设置对比度 (-2到2, 0为默认)  
        camera.set_contrast(0)
        print("✓ 对比度设置为: 0")
        
        # 设置饱和度 (-2到2, 0为默认)
        camera.set_saturation(0)
        print("✓ 饱和度设置为: 0")
        
    except Exception as e:
        print(f"⚠ 设置调整失败: {e}")
        print("注意: 如果固件不包含相机驱动，此步骤会失败")

def capture_photo():
    """拍摄照片"""
    print("\n=== 拍摄照片 ===")
    
    try:
        print("拍摄中...")
        buf = camera.capture()
        
        if buf and len(buf) > 0:
            print(f"✓ 拍摄成功! 图像大小: {len(buf)} 字节")
            return buf
        else:
            print("✗ 拍摄失败: 返回空数据")
            return None
            
    except Exception as e:
        print(f"✗ 拍摄失败: {e}")
        return None

def save_photo(image_data, filename=None):
    """保存照片到文件系统"""
    print("\n=== 保存照片 ===")
    
    if not image_data:
        print("✗ 没有图像数据可保存")
        return False
    
    # 生成文件名
    if filename is None:
        timestamp = time.ticks_ms()
        filename = f"photo_{timestamp}.jpg"
    
    try:
        # 确保文件名以.jpg结尾
        if not filename.endswith('.jpg'):
            filename += '.jpg'
            
        # 保存到文件
        with open(filename, 'wb') as f:
            f.write(image_data)
            
        print(f"✓ 照片已保存为: {filename}")
        print(f"文件大小: {len(image_data)} 字节")
        
        return True
        
    except Exception as e:
        print(f"✗ 保存失败: {e}")
        return False

def list_saved_photos():
    """列出已保存的照片"""
    print("\n=== 已保存的照片 ===")
    
    try:
        files = os.listdir('.')
        photo_files = [f for f in files if f.endswith('.jpg')]
        
        if photo_files:
            print("找到以下照片文件:")
            for i, filename in enumerate(photo_files, 1):
                try:
                    stat = os.stat(filename)
                    size = stat[6]  # 文件大小
                    print(f"  {i}. {filename} ({size} 字节)")
                except:
                    print(f"  {i}. {filename}")
        else:
            print("没有找到照片文件")
            
    except Exception as e:
        print(f"列出文件时出错: {e}")

def capture_multiple_photos(count=3):
    """连续拍摄多张照片"""
    print(f"\n=== 连续拍摄 {count} 张照片 ===")
    
    successful_captures = 0
    
    for i in range(count):
        print(f"\n拍摄第 {i+1}/{count} 张...")
        
        # 拍摄
        image_data = capture_photo()
        
        if image_data:
            # 保存
            filename = f"photo_{i+1}_{time.ticks_ms()}.jpg"
            if save_photo(image_data, filename):
                successful_captures += 1
        
        # 等待间隔
        if i < count - 1:
            print("等待 2 秒...")
            time.sleep(2)
    
    print(f"\n✓ 成功拍摄并保存 {successful_captures}/{count} 张照片")

def camera_status_check():
    """检查相机状态"""
    print("\n=== 相机状态检查 ===")
    
    status = camera.get_status()
    print("相机状态:")
    for key, value in status.items():
        print(f"  {key}: {value}")

def main():
    """主函数 - 完整的相机使用流程"""
    print("ESP32-S3 相机使用完整示例")
    print("=" * 40)
    
    try:
        # 1. 设置引脚
        setup_camera_pins()
        
        # 2. 检查状态
        camera_status_check()
        
        # 3. 初始化相机
        if not initialize_camera():
            print("相机初始化失败，退出")
            return
        
        # 4. 调整设置
        adjust_camera_settings()
        
        # 5. 拍摄单张照片
        print("\n" + "=" * 20)
        print("开始拍摄测试...")
        
        image_data = capture_photo()
        if image_data:
            save_photo(image_data, "test_photo.jpg")
        
        # 6. 连续拍摄多张照片
        user_input = input("\n是否要连续拍摄3张照片? (y/n): ")
        if user_input.lower() == 'y':
            capture_multiple_photos(3)
        
        # 7. 列出保存的照片
        list_saved_photos()
        
        print("\n✓ 相机测试完成!")
        
    except KeyboardInterrupt:
        print("\n用户中断操作")
    except Exception as e:
        print(f"\n程序执行出错: {e}")
    finally:
        # 清理资源
        try:
            camera.deinit()
            print("相机资源已清理")
        except:
            pass

if __name__ == "__main__":
    main()