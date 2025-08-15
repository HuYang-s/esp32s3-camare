"""
ESP32-S3 Enhanced Camera Test Script
增强相机模块测试脚本

This script demonstrates and tests the enhanced camera functionality
with dynamic pin configuration.
"""

import time
import gc
try:
    import camera_enhanced as camera
    print("✓ Enhanced camera module imported successfully")
except ImportError as e:
    print("✗ Failed to import enhanced camera module:", e)
    print("Please ensure camera_enhanced.py is uploaded to the device")
    exit()

def print_separator(title=""):
    print("=" * 50)
    if title:
        print(f" {title} ")
        print("=" * 50)

def test_basic_functionality():
    """Test basic camera functionality"""
    print_separator("Basic Functionality Test")
    
    try:
        # Test status
        status = camera.get_status()
        print(f"Camera status: {status}")
        
        # Test available presets
        presets = camera.list_presets()
        print(f"Available presets: {presets}")
        
        # Test current pin configuration
        current_pins = camera.get_pins()
        print("Current pin configuration:")
        for pin_name, pin_num in current_pins.items():
            print(f"  {pin_name}: {pin_num}")
            
        print("✓ Basic functionality test passed")
        
    except Exception as e:
        print(f"✗ Basic functionality test failed: {e}")
        return False
    
    return True

def test_preset_loading():
    """Test preset configuration loading"""
    print_separator("Preset Loading Test")
    
    try:
        # Test loading different presets
        presets_to_test = ['ESP32-S3-CAM', 'ESP32-CAM', 'TTGO-T-Camera']
        
        for preset in presets_to_test:
            try:
                print(f"Testing preset: {preset}")
                camera.load_preset(preset)
                pins = camera.get_pins()
                print(f"  Loaded pins: {len(pins)} pins configured")
                print("  ✓ Preset loaded successfully")
            except Exception as e:
                print(f"  ✗ Failed to load preset {preset}: {e}")
        
        # Reset to default
        camera.load_preset('ESP32-S3-CAM')
        print("✓ Preset loading test completed")
        
    except Exception as e:
        print(f"✗ Preset loading test failed: {e}")
        return False
    
    return True

def test_custom_pin_configuration():
    """Test custom pin configuration"""
    print_separator("Custom Pin Configuration Test")
    
    try:
        # Save current configuration
        original_pins = camera.get_pins().copy()
        
        # Test custom configuration
        custom_pins = {
            'xclk': 15,
            'siod': 4,
            'sioc': 5,
            'y9': 16,
            'y8': 17,
            'y7': 18,
            'y6': 12,
            'y5': 10,
            'y4': 8,
            'y3': 9,
            'y2': 11,
            'vsync': 6,
            'href': 7,
            'pclk': 13,
            'pwdn': 43,
            'reset': 44
        }
        
        print("Setting custom pin configuration...")
        camera.set_pins(custom_pins)
        
        # Verify configuration
        current_pins = camera.get_pins()
        for pin_name, expected_pin in custom_pins.items():
            if current_pins.get(pin_name) != expected_pin:
                raise Exception(f"Pin {pin_name} not set correctly")
        
        print("✓ Custom pin configuration set successfully")
        
        # Restore original configuration
        camera.set_pins(original_pins)
        print("✓ Original configuration restored")
        
    except Exception as e:
        print(f"✗ Custom pin configuration test failed: {e}")
        return False
    
    return True

def test_camera_initialization():
    """Test camera initialization with different configurations"""
    print_separator("Camera Initialization Test")
    
    try:
        # Test initialization with default settings
        print("Initializing camera with default settings...")
        camera.init()
        print("✓ Camera initialized successfully")
        
        # Test camera status after initialization
        status = camera.get_status()
        print(f"Camera status after init: {status}")
        
        # Test deinitialization
        print("Deinitializing camera...")
        camera.deinit()
        print("✓ Camera deinitialized successfully")
        
    except Exception as e:
        print(f"✗ Camera initialization test failed: {e}")
        return False
    
    return True

def test_camera_capture():
    """Test camera capture functionality"""
    print_separator("Camera Capture Test")
    
    try:
        # Initialize camera
        print("Initializing camera for capture test...")
        camera.init()
        
        # Wait for camera to stabilize
        time.sleep(2)
        
        # Test capture
        print("Capturing image...")
        buf = camera.capture()
        
        if buf and len(buf) > 0:
            print(f"✓ Image captured successfully: {len(buf)} bytes")
            
            # Test multiple captures
            print("Testing multiple captures...")
            for i in range(3):
                buf = camera.capture()
                print(f"  Capture {i+1}: {len(buf)} bytes")
                time.sleep(1)
            
            print("✓ Multiple captures successful")
        else:
            raise Exception("Capture returned empty buffer")
        
        # Clean up
        camera.deinit()
        
    except Exception as e:
        print(f"✗ Camera capture test failed: {e}")
        return False
    
    return True

def test_camera_settings():
    """Test camera settings adjustment"""
    print_separator("Camera Settings Test")
    
    try:
        # Initialize camera
        camera.init()
        
        # Test brightness adjustment
        print("Testing brightness adjustment...")
        for brightness in [-2, 0, 2]:
            camera.set_brightness(brightness)
            print(f"  Set brightness to: {brightness}")
        
        # Test contrast adjustment
        print("Testing contrast adjustment...")
        for contrast in [-2, 0, 2]:
            camera.set_contrast(contrast)
            print(f"  Set contrast to: {contrast}")
        
        # Test saturation adjustment
        print("Testing saturation adjustment...")
        for saturation in [-2, 0, 2]:
            camera.set_saturation(saturation)
            print(f"  Set saturation to: {saturation}")
        
        print("✓ Camera settings test passed")
        
        # Clean up
        camera.deinit()
        
    except Exception as e:
        print(f"✗ Camera settings test failed: {e}")
        return False
    
    return True

def test_error_handling():
    """Test error handling"""
    print_separator("Error Handling Test")
    
    try:
        # Test invalid pin configuration
        print("Testing invalid pin configuration...")
        try:
            invalid_pins = {'xclk': 99}  # Invalid pin number
            camera.set_pins(invalid_pins)
            print("  ✗ Should have raised an error for invalid pin")
            return False
        except Exception as e:
            print(f"  ✓ Correctly caught invalid pin error: {e}")
        
        # Test capture without initialization
        print("Testing capture without initialization...")
        try:
            camera.deinit()  # Ensure camera is not initialized
            camera.capture()
            print("  ✗ Should have raised an error for uninitialized camera")
            return False
        except Exception as e:
            print(f"  ✓ Correctly caught uninitialized camera error: {e}")
        
        print("✓ Error handling test passed")
        
    except Exception as e:
        print(f"✗ Error handling test failed: {e}")
        return False
    
    return True

def run_comprehensive_test():
    """Run comprehensive test suite"""
    print_separator("ESP32-S3 Enhanced Camera Comprehensive Test")
    
    print("Starting comprehensive test suite...")
    print("This will test all enhanced camera functionality")
    print()
    
    # Test results
    tests = [
        ("Basic Functionality", test_basic_functionality),
        ("Preset Loading", test_preset_loading),
        ("Custom Pin Configuration", test_custom_pin_configuration),
        ("Camera Initialization", test_camera_initialization),
        ("Camera Capture", test_camera_capture),
        ("Camera Settings", test_camera_settings),
        ("Error Handling", test_error_handling),
    ]
    
    results = []
    
    for test_name, test_func in tests:
        print(f"\n--- Running {test_name} Test ---")
        try:
            result = test_func()
            results.append((test_name, result))
            if result:
                print(f"✓ {test_name} test PASSED")
            else:
                print(f"✗ {test_name} test FAILED")
        except Exception as e:
            print(f"✗ {test_name} test FAILED with exception: {e}")
            results.append((test_name, False))
        
        # Clean up memory
        gc.collect()
        time.sleep(1)
    
    # Print summary
    print_separator("Test Summary")
    passed = sum(1 for _, result in results if result)
    total = len(results)
    
    print(f"Tests passed: {passed}/{total}")
    print()
    
    for test_name, result in results:
        status = "PASS" if result else "FAIL"
        symbol = "✓" if result else "✗"
        print(f"{symbol} {test_name}: {status}")
    
    if passed == total:
        print("\n🎉 All tests passed! Enhanced camera is working correctly.")
    else:
        print(f"\n⚠️  {total - passed} test(s) failed. Please check the errors above.")
    
    return passed == total

def main():
    """Main function"""
    try:
        print("ESP32-S3 Enhanced Camera Test Script")
        print("增强相机模块测试脚本")
        print()
        
        # Run comprehensive test
        success = run_comprehensive_test()
        
        if success:
            print("\n✓ Enhanced camera module is ready for use!")
        else:
            print("\n✗ Some tests failed. Please check the configuration.")
            
    except KeyboardInterrupt:
        print("\n\nTest interrupted by user")
    except Exception as e:
        print(f"\nTest suite failed with error: {e}")
    finally:
        # Ensure camera is properly cleaned up
        try:
            camera.deinit()
        except:
            pass

if __name__ == "__main__":
    main()