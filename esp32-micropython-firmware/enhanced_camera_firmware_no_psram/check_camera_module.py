"""
检查ESP32-S3固件中的相机模块支持
Check Camera Module Support in ESP32-S3 Firmware
"""

import sys
import gc

def check_camera_availability():
    """检查相机模块可用性"""
    print("=== ESP32-S3 相机模块检查 ===")
    
    # 检查原生camera模块
    print("1. 检查原生camera模块...")
    try:
        import camera
        print("✓ 原生camera模块可用")
        
        # 检查camera模块的属性
        print("  可用属性:")
        attrs = dir(camera)
        for attr in attrs:
            if not attr.startswith('_'):
                print(f"    - {attr}")
        
        return True, camera
        
    except ImportError as e:
        print(f"✗ 原生camera模块不可用: {e}")
        return False, None

def check_system_info():
    """检查系统信息"""
    print("\n=== 系统信息 ===")
    
    # 检查MicroPython版本
    print(f"MicroPython版本: {sys.version}")
    
    # 检查可用模块
    print("\n可用的内置模块:")
    try:
        help('modules')
    except:
        # 如果help不可用，列出sys.modules
        print("sys.modules中的模块:")
        for module in sorted(sys.modules.keys()):
            print(f"  - {module}")

def check_memory():
    """检查内存信息"""
    print("\n=== 内存信息 ===")
    
    try:
        import esp
        print(f"总堆大小: {esp.heap_size():,} 字节")
        print(f"可用堆大小: {esp.heap_free():,} 字节")
        
        # 判断是否有PSRAM
        if esp.heap_size() > 400000:
            print("✓ 可能有PSRAM支持")
        else:
            print("⚠ 可能没有PSRAM")
            
    except ImportError:
        print("esp模块不可用")
    
    # 垃圾回收信息
    gc.collect()
    print(f"垃圾回收后可用内存: {gc.mem_free():,} 字节")

def check_gpio_modules():
    """检查GPIO相关模块"""
    print("\n=== GPIO模块检查 ===")
    
    gpio_modules = ['machine', 'Pin']
    
    for module_name in gpio_modules:
        try:
            if module_name == 'Pin':
                from machine import Pin
                print(f"✓ {module_name} 可用")
                
                # 测试创建一个Pin对象
                test_pin = Pin(2, Pin.OUT)
                test_pin.deinit()
                print("  Pin对象创建测试成功")
                
            else:
                exec(f"import {module_name}")
                print(f"✓ {module_name} 模块可用")
                
        except Exception as e:
            print(f"✗ {module_name} 不可用: {e}")

def suggest_solutions():
    """建议解决方案"""
    print("\n=== 解决方案建议 ===")
    
    print("由于原生camera模块不可用，您有以下选择:")
    print()
    
    print("1. 🔄 使用带相机支持的固件")
    print("   - 刷写 enhanced_camera_firmware 中的固件")
    print("   - 这些固件包含相机驱动支持")
    print("   - 需要确保硬件有PSRAM支持")
    print()
    
    print("2. 🛠 自定义编译固件")
    print("   - 使用ESP-IDF编译包含相机支持的MicroPython")
    print("   - 根据您的硬件配置定制固件")
    print("   - 适合有经验的开发者")
    print()
    
    print("3. 📦 使用外部相机库")
    print("   - 寻找纯Python实现的相机库")
    print("   - 可能性能较低，但兼容性更好")
    print("   - 适合简单应用")
    print()
    
    print("4. 🔌 直接GPIO控制")
    print("   - 使用machine.Pin直接控制相机引脚")
    print("   - 需要深入了解相机协议")
    print("   - 适合高级用户")

def main():
    """主检查函数"""
    print("ESP32-S3 相机模块诊断工具")
    print("=" * 40)
    
    # 检查相机可用性
    camera_available, camera_module = check_camera_availability()
    
    # 检查系统信息
    check_system_info()
    
    # 检查内存
    check_memory()
    
    # 检查GPIO模块
    check_gpio_modules()
    
    # 如果相机不可用，提供建议
    if not camera_available:
        suggest_solutions()
    else:
        print("\n✓ 相机模块可用，您可以正常使用camera_enhanced.py")
    
    print("\n" + "=" * 40)
    print("诊断完成")

if __name__ == "__main__":
    main()