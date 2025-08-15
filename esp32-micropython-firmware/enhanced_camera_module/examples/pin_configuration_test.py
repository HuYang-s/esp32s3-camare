"""
Enhanced ESP32-S3 相机模块引脚配置测试脚本

此脚本演示如何使用增强相机模块的动态引脚配置功能
"""

import camera
import time

def test_pin_configuration():
    """测试引脚配置功能"""
    print("=== 引脚配置功能测试 ===")
    
    # 1. 查看默认引脚配置
    print("\n1. 查看默认引脚配置:")
    default_pins = camera.get_pins()
    for pin_name, pin_num in default_pins.items():
        print(f"  {pin_name}: {pin_num}")
    
    # 2. 测试自定义引脚配置
    print("\n2. 设置自定义引脚配置:")
    custom_pins = {
        'pwdn': 32,
        'reset': -1,
        'xclk': 0,
        'sda': 26,
        'scl': 27,
        'd7': 35,
        'd6': 34,
        'd5': 39,
        'd4': 36,
        'd3': 21,
        'd2': 19,
        'd1': 18,
        'd0': 5,
        'vsync': 25,
        'href': 23,
        'pclk': 22
    }
    
    camera.set_pins(custom_pins)
    print("  ✓ 自定义引脚配置已设置")
    
    # 3. 验证引脚配置
    print("\n3. 验证引脚配置:")
    current_pins = camera.get_pins()
    for pin_name, pin_num in current_pins.items():
        expected = custom_pins.get(pin_name, -1)
        status = "✓" if pin_num == expected else "✗"
        print(f"  {status} {pin_name}: {pin_num} (期望: {expected})")

def test_camera_initialization():
    """测试相机初始化"""
    print("\n=== 相机初始化测试 ===")
    
    try:
        # 使用默认参数初始化
        print("\n1. 使用默认参数初始化相机...")
        camera.init()
        print("  ✓ 相机初始化成功")
        
        # 测试拍照
        print("\n2. 测试拍照功能...")
        img = camera.capture()
        if img:
            print(f"  ✓ 拍照成功，图像大小: {len(img)} bytes")
        else:
            print("  ✗ 拍照失败")
        
        # 释放相机
        camera.deinit()
        print("  ✓ 相机资源已释放")
        
    except Exception as e:
        print(f"  ✗ 相机测试失败: {e}")

def test_different_configurations():
    """测试不同的相机配置"""
    print("\n=== 不同配置测试 ===")
    
    configurations = [
        {
            'name': 'QVGA低质量',
            'params': {
                'framesize': camera.FRAME_QVGA,
                'quality': 30
            }
        },
        {
            'name': 'VGA中等质量',
            'params': {
                'framesize': camera.FRAME_VGA,
                'quality': 15
            }
        },
        {
            'name': 'QQVGA高质量',
            'params': {
                'framesize': camera.FRAME_QQVGA,
                'quality': 8
            }
        }
    ]
    
    for config in configurations:
        print(f"\n测试配置: {config['name']}")
        try:
            camera.init(**config['params'])
            print(f"  ✓ 初始化成功")
            
            img = camera.capture()
            if img:
                print(f"  ✓ 拍照成功，大小: {len(img)} bytes")
            else:
                print(f"  ✗ 拍照失败")
                
            camera.deinit()
            time.sleep(1)  # 等待一秒再测试下一个配置
            
        except Exception as e:
            print(f"  ✗ 配置测试失败: {e}")
            try:
                camera.deinit()
            except:
                pass

def test_camera_settings():
    """测试相机设置调整"""
    print("\n=== 相机设置调整测试 ===")
    
    try:
        # 初始化相机
        camera.init(framesize=camera.FRAME_QVGA, quality=20)
        print("  ✓ 相机初始化成功")
        
        # 测试不同的亮度设置
        brightness_levels = [-2, -1, 0, 1, 2]
        print("\n测试亮度调整:")
        for brightness in brightness_levels:
            camera.set_brightness(brightness)
            img = camera.capture()
            print(f"  亮度 {brightness:2d}: {len(img)} bytes")
        
        # 重置为默认设置
        camera.set_brightness(0)
        camera.set_contrast(0)
        camera.set_saturation(0)
        print("  ✓ 相机设置已重置为默认值")
        
        camera.deinit()
        
    except Exception as e:
        print(f"  ✗ 相机设置测试失败: {e}")
        try:
            camera.deinit()
        except:
            pass

def test_pin_validation():
    """测试引脚配置验证"""
    print("\n=== 引脚配置验证测试 ===")
    
    # 测试无效的引脚配置
    invalid_configs = [
        {'invalid_pin': 10},  # 无效的引脚名称
        {'xclk': 999},        # 无效的引脚号
    ]
    
    for i, invalid_config in enumerate(invalid_configs, 1):
        print(f"\n{i}. 测试无效配置: {invalid_config}")
        try:
            camera.set_pins(invalid_config)
            print("  ⚠️ 无效配置被接受了（可能需要改进验证）")
        except Exception as e:
            print(f"  ✓ 正确拒绝了无效配置: {e}")

def comprehensive_test():
    """综合测试"""
    print("=== ESP32-S3 增强相机模块综合测试 ===")
    print("支持动态引脚配置的相机模块测试")
    print("=" * 50)
    
    # 检查模块功能
    print("检查模块功能:")
    features = [
        ('set_pins', '动态引脚配置'),
        ('get_pins', '引脚状态查询'),
        ('init', '相机初始化'),
        ('capture', '图像拍摄'),
        ('deinit', '资源释放'),
        ('set_brightness', '亮度调整'),
        ('set_contrast', '对比度调整'),
        ('set_saturation', '饱和度调整')
    ]
    
    for func_name, description in features:
        if hasattr(camera, func_name):
            print(f"  ✓ {description} ({func_name})")
        else:
            print(f"  ✗ {description} ({func_name}) - 功能缺失")
    
    print()
    
    # 运行所有测试
    try:
        test_pin_configuration()
        test_camera_initialization()
        test_different_configurations()
        test_camera_settings()
        test_pin_validation()
        
        print("\n" + "=" * 50)
        print("🎉 所有测试完成！")
        print("增强相机模块功能正常")
        
    except Exception as e:
        print(f"\n❌ 测试过程中发生错误: {e}")
        print("请检查硬件连接和固件版本")

def interactive_pin_config():
    """交互式引脚配置"""
    print("\n=== 交互式引脚配置 ===")
    print("当前引脚配置:")
    
    current_pins = camera.get_pins()
    pin_names = list(current_pins.keys())
    
    for i, (pin_name, pin_num) in enumerate(current_pins.items()):
        print(f"  {i+1:2d}. {pin_name:6s}: {pin_num}")
    
    print("\n输入要修改的引脚编号 (1-16)，或输入0退出:")
    
    try:
        choice = input("选择: ")
        if choice == '0':
            return
        
        choice = int(choice) - 1
        if 0 <= choice < len(pin_names):
            pin_name = pin_names[choice]
            current_value = current_pins[pin_name]
            
            new_value = input(f"输入 {pin_name} 的新引脚号 (当前: {current_value}): ")
            new_value = int(new_value)
            
            # 更新引脚配置
            new_pins = current_pins.copy()
            new_pins[pin_name] = new_value
            
            camera.set_pins(new_pins)
            print(f"✓ {pin_name} 已更新为 {new_value}")
            
            # 显示更新后的配置
            updated_pins = camera.get_pins()
            print("\n更新后的引脚配置:")
            for name, num in updated_pins.items():
                marker = " *" if name == pin_name else ""
                print(f"  {name:6s}: {num}{marker}")
        else:
            print("无效的选择")
            
    except (ValueError, KeyboardInterrupt):
        print("操作已取消")

if __name__ == "__main__":
    # 运行综合测试
    comprehensive_test()
    
    # 可选：运行交互式配置
    # interactive_pin_config()