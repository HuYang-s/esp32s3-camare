"""
ESP32-S3 相机功能测试示例

此脚本演示如何在ESP32-S3上使用MicroPython相机模块
"""

import camera
import time

def test_camera():
    """测试相机基本功能"""
    print("开始相机测试...")
    
    try:
        # 初始化相机
        print("初始化相机...")
        camera.init()
        print("相机初始化成功")
        
        # 拍照测试
        print("拍照中...")
        img = camera.capture()
        
        if img:
            print(f"拍照成功！图像大小: {len(img)} bytes")
            
            # 可以将图像数据保存到文件或通过网络发送
            # 这里仅打印前20个字节作为示例
            print(f"图像数据前20字节: {img[:20]}")
        else:
            print("拍照失败")
            
    except Exception as e:
        print(f"相机测试出错: {e}")
        
    finally:
        # 释放相机资源
        try:
            camera.deinit()
            print("相机资源已释放")
        except:
            pass

def continuous_capture(count=5, interval=2):
    """连续拍照测试"""
    print(f"连续拍照测试，共{count}张，间隔{interval}秒")
    
    try:
        camera.init()
        
        for i in range(count):
            print(f"拍照 {i+1}/{count}...")
            img = camera.capture()
            
            if img:
                print(f"  成功，大小: {len(img)} bytes")
            else:
                print(f"  失败")
                
            if i < count - 1:  # 最后一张不需要等待
                time.sleep(interval)
                
    except Exception as e:
        print(f"连续拍照出错: {e}")
        
    finally:
        try:
            camera.deinit()
        except:
            pass

if __name__ == "__main__":
    # 基础测试
    test_camera()
    
    print("\n" + "="*40 + "\n")
    
    # 连续拍照测试
    continuous_capture(3, 1)