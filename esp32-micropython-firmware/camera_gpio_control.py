"""
纯GPIO相机控制模块 (备用方案)
Pure GPIO Camera Control Module (Fallback Solution)

当原生camera模块不可用时，这个模块提供基本的GPIO引脚控制功能。
注意: 这不能实现完整的相机功能，但可以进行引脚管理和基本控制。
"""

from machine import Pin, I2C
import time

class GPIOCameraController:
    """基于GPIO的相机控制器"""
    
    def __init__(self):
        self.pins = {}
        self.i2c = None
        self.initialized = False
        self.pin_config = None
        
    def set_pins(self, pin_config):
        """设置相机引脚配置"""
        print("设置GPIO引脚配置...")
        
        self.pin_config = pin_config.copy()
        
        # 清理现有引脚
        self.cleanup_pins()
        
        try:
            # 设置控制引脚
            if pin_config.get('pwdn', -1) >= 0:
                self.pins['pwdn'] = Pin(pin_config['pwdn'], Pin.OUT)
                self.pins['pwdn'].value(0)  # 激活相机 (PWDN通常是低电平有效)
                print(f"  PWDN: GPIO{pin_config['pwdn']} (设置为低电平)")
            
            if pin_config.get('reset', -1) >= 0:
                self.pins['reset'] = Pin(pin_config['reset'], Pin.OUT)
                self.pins['reset'].value(1)  # 保持高电平，不复位
                print(f"  RESET: GPIO{pin_config['reset']} (设置为高电平)")
            
            # 设置时钟引脚
            if pin_config.get('xclk', -1) >= 0:
                self.pins['xclk'] = Pin(pin_config['xclk'], Pin.OUT)
                print(f"  XCLK: GPIO{pin_config['xclk']} (输出)")
            
            # 设置I2C引脚 (用于相机配置)
            sda_pin = pin_config.get('sda', -1)
            scl_pin = pin_config.get('scl', -1)
            
            if sda_pin >= 0 and scl_pin >= 0:
                self.i2c = I2C(0, sda=Pin(sda_pin), scl=Pin(scl_pin), freq=100000)
                print(f"  I2C: SDA=GPIO{sda_pin}, SCL=GPIO{scl_pin}")
            
            # 设置数据引脚
            for i in range(8):
                pin_name = f'd{i}'
                if pin_config.get(pin_name, -1) >= 0:
                    self.pins[pin_name] = Pin(pin_config[pin_name], Pin.IN)
                    print(f"  D{i}: GPIO{pin_config[pin_name]} (输入)")
            
            # 设置同步引脚
            sync_pins = ['vsync', 'href', 'pclk']
            for pin_name in sync_pins:
                if pin_config.get(pin_name, -1) >= 0:
                    self.pins[pin_name] = Pin(pin_config[pin_name], Pin.IN)
                    print(f"  {pin_name.upper()}: GPIO{pin_config[pin_name]} (输入)")
            
            print("✓ GPIO引脚配置完成")
            return True
            
        except Exception as e:
            print(f"✗ GPIO配置失败: {e}")
            self.cleanup_pins()
            return False
    
    def cleanup_pins(self):
        """清理GPIO引脚"""
        for pin in self.pins.values():
            try:
                pin.deinit()
            except:
                pass
        self.pins.clear()
        
        if self.i2c:
            try:
                self.i2c.deinit()
            except:
                pass
            self.i2c = None
    
    def generate_clock(self, frequency=10000000, duration=1.0):
        """生成时钟信号 (仅用于测试)"""
        if 'xclk' not in self.pins:
            print("✗ XCLK引脚未配置")
            return False
        
        print(f"生成 {frequency}Hz 时钟信号，持续 {duration} 秒...")
        
        xclk_pin = self.pins['xclk']
        half_period_us = int(500000 / frequency)  # 微秒
        
        start_time = time.ticks_ms()
        
        try:
            while time.ticks_diff(time.ticks_ms(), start_time) < duration * 1000:
                xclk_pin.value(1)
                time.sleep_us(half_period_us)
                xclk_pin.value(0)
                time.sleep_us(half_period_us)
            
            print("✓ 时钟信号生成完成")
            return True
            
        except KeyboardInterrupt:
            print("时钟信号生成被中断")
            xclk_pin.value(0)
            return False
    
    def scan_i2c(self):
        """扫描I2C设备"""
        if not self.i2c:
            print("✗ I2C未初始化")
            return []
        
        print("扫描I2C设备...")
        
        try:
            devices = self.i2c.scan()
            
            if devices:
                print(f"找到 {len(devices)} 个I2C设备:")
                for addr in devices:
                    print(f"  - 地址: 0x{addr:02X} ({addr})")
                    
                    # 检查是否是常见的相机芯片
                    if addr == 0x30:
                        print("    可能是OV2640相机")
                    elif addr == 0x3C:
                        print("    可能是OV3660相机")
                    elif addr == 0x21:
                        print("    可能是OV5640相机")
            else:
                print("未找到I2C设备")
            
            return devices
            
        except Exception as e:
            print(f"✗ I2C扫描失败: {e}")
            return []
    
    def read_pin_states(self):
        """读取所有引脚状态"""
        print("=== 引脚状态读取 ===")
        
        if not self.pins:
            print("✗ 没有配置的引脚")
            return {}
        
        states = {}
        
        # 读取输入引脚状态
        input_pins = ['vsync', 'href', 'pclk'] + [f'd{i}' for i in range(8)]
        
        for pin_name in input_pins:
            if pin_name in self.pins:
                try:
                    value = self.pins[pin_name].value()
                    states[pin_name] = value
                    print(f"  {pin_name.upper()}: {value}")
                except Exception as e:
                    print(f"  {pin_name.upper()}: 读取失败 ({e})")
                    states[pin_name] = None
        
        # 显示输出引脚状态
        output_pins = ['pwdn', 'reset', 'xclk']
        for pin_name in output_pins:
            if pin_name in self.pins:
                try:
                    value = self.pins[pin_name].value()
                    states[pin_name] = value
                    print(f"  {pin_name.upper()}: {value} (输出)")
                except Exception as e:
                    print(f"  {pin_name.upper()}: 读取失败 ({e})")
                    states[pin_name] = None
        
        return states
    
    def test_camera_presence(self):
        """测试相机是否存在"""
        print("=== 相机存在性测试 ===")
        
        # 1. 检查I2C通信
        print("1. 检查I2C通信...")
        devices = self.scan_i2c()
        i2c_ok = len(devices) > 0
        
        # 2. 检查引脚状态
        print("\n2. 检查引脚状态...")
        states = self.read_pin_states()
        
        # 3. 生成短暂的时钟信号
        print("\n3. 测试时钟输出...")
        clock_ok = self.generate_clock(frequency=1000000, duration=0.1)
        
        # 总结
        print(f"\n=== 测试结果 ===")
        print(f"I2C通信: {'✓' if i2c_ok else '✗'}")
        print(f"时钟输出: {'✓' if clock_ok else '✗'}")
        print(f"引脚配置: {'✓' if self.pins else '✗'}")
        
        if i2c_ok:
            print("\n✓ 检测到可能的相机设备")
            print("建议: 尝试使用带相机支持的固件")
        else:
            print("\n⚠ 未检测到相机设备")
            print("可能原因:")
            print("  - 相机模块未连接")
            print("  - 引脚配置错误")
            print("  - 相机模块故障")
        
        return i2c_ok and clock_ok
    
    def get_status(self):
        """获取控制器状态"""
        return {
            'pins_configured': len(self.pins),
            'i2c_available': self.i2c is not None,
            'pin_config': self.pin_config.copy() if self.pin_config else None
        }

def create_ov2640_controller():
    """创建OV2640专用控制器"""
    print("创建OV2640 GPIO控制器...")
    
    controller = GPIOCameraController()
    
    # OV2640引脚配置
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
    
    if controller.set_pins(ov2640_pins):
        print("✓ OV2640控制器创建成功")
        return controller
    else:
        print("✗ OV2640控制器创建失败")
        return None

def main():
    """GPIO相机控制演示"""
    print("GPIO相机控制模块演示")
    print("=" * 30)
    
    # 创建OV2640控制器
    controller = create_ov2640_controller()
    
    if controller:
        # 测试相机存在性
        controller.test_camera_presence()
        
        # 显示状态
        status = controller.get_status()
        print(f"\n控制器状态: {status}")
        
        # 清理
        controller.cleanup_pins()
        print("\n✓ 资源已清理")
    
    print("\n注意: 这只是GPIO级别的控制")
    print("要实现完整相机功能，需要使用带相机支持的固件")

if __name__ == "__main__":
    main()