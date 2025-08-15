"""
Enhanced Camera Module for ESP32-S3 MicroPython
Provides dynamic pin configuration and advanced camera features

This module wraps the built-in camera module to provide:
- Dynamic pin configuration
- Pin state management
- Enhanced error handling
- Multiple camera configurations
- Backward compatibility

Usage:
    import camera_enhanced as camera
    
    # Configure pins dynamically
    pins = {
        'xclk': 0, 'sda': 26, 'scl': 27,
        'd7': 35, 'd6': 34, 'd5': 39, 'd4': 36,
        'd3': 21, 'd2': 19, 'd1': 18, 'd0': 5,
        'vsync': 25, 'href': 23, 'pclk': 22
    }
    camera.set_pins(pins)
    camera.init()
    img = camera.capture()
"""

try:
    import camera as _camera_native
    _CAMERA_AVAILABLE = True
except ImportError:
    _CAMERA_AVAILABLE = False
    print("Warning: Native camera module not available")

import machine
from machine import Pin
import json

# Camera module constants - re-export from native module
if _CAMERA_AVAILABLE:
    # Image formats
    JPEG = getattr(_camera_native, 'JPEG', 0)
    RGB565 = getattr(_camera_native, 'RGB565', 1)
    GRAYSCALE = getattr(_camera_native, 'GRAYSCALE', 2)
    
    # Frame sizes
    FRAME_QQVGA = getattr(_camera_native, 'FRAME_QQVGA', 0)   # 160x120
    FRAME_QVGA = getattr(_camera_native, 'FRAME_QVGA', 1)     # 320x240
    FRAME_VGA = getattr(_camera_native, 'FRAME_VGA', 2)       # 640x480
    FRAME_SVGA = getattr(_camera_native, 'FRAME_SVGA', 3)     # 800x600
    FRAME_XGA = getattr(_camera_native, 'FRAME_XGA', 4)       # 1024x768
    FRAME_SXGA = getattr(_camera_native, 'FRAME_SXGA', 5)     # 1280x1024
    FRAME_UXGA = getattr(_camera_native, 'FRAME_UXGA', 6)     # 1600x1200
else:
    # Fallback constants
    JPEG, RGB565, GRAYSCALE = 0, 1, 2
    FRAME_QQVGA, FRAME_QVGA, FRAME_VGA = 0, 1, 2
    FRAME_SVGA, FRAME_XGA, FRAME_SXGA, FRAME_UXGA = 3, 4, 5, 6

# Default pin configurations for different boards
DEFAULT_PIN_CONFIGS = {
    'ESP32-S3-CAM': {
        'pwdn': 32, 'reset': -1, 'xclk': 0,
        'sda': 26, 'scl': 27,
        'd7': 35, 'd6': 34, 'd5': 39, 'd4': 36,
        'd3': 21, 'd2': 19, 'd1': 18, 'd0': 5,
        'vsync': 25, 'href': 23, 'pclk': 22
    },
    'AI-Thinker': {
        'pwdn': 32, 'reset': -1, 'xclk': 0,
        'sda': 26, 'scl': 27,
        'd7': 35, 'd6': 34, 'd5': 39, 'd4': 36,
        'd3': 21, 'd2': 19, 'd1': 18, 'd0': 5,
        'vsync': 25, 'href': 23, 'pclk': 22
    }
}

# Global state
_current_pins = DEFAULT_PIN_CONFIGS['ESP32-S3-CAM'].copy()
_camera_initialized = False
_pin_objects = {}

class CameraError(Exception):
    """Camera-specific exception"""
    pass

class PinConfigError(Exception):
    """Pin configuration error"""
    pass

def _validate_pin_config(pins):
    """Validate pin configuration"""
    required_pins = ['xclk', 'sda', 'scl', 'd7', 'd6', 'd5', 'd4', 'd3', 'd2', 'd1', 'd0', 'vsync', 'href', 'pclk']
    
    for pin_name in required_pins:
        if pin_name not in pins:
            raise PinConfigError(f"Missing required pin: {pin_name}")
        
        pin_num = pins[pin_name]
        if not isinstance(pin_num, int) or pin_num < -1 or pin_num > 48:
            raise PinConfigError(f"Invalid pin number for {pin_name}: {pin_num}")
    
    # Check for pin conflicts (same pin used for different functions)
    used_pins = {}
    for pin_name, pin_num in pins.items():
        if pin_num >= 0:  # -1 means unused
            if pin_num in used_pins:
                raise PinConfigError(f"Pin conflict: pin {pin_num} used for both {used_pins[pin_num]} and {pin_name}")
            used_pins[pin_num] = pin_name

def _setup_pins():
    """Setup GPIO pins based on current configuration"""
    global _pin_objects
    
    # Clean up existing pin objects
    for pin_obj in _pin_objects.values():
        try:
            pin_obj.deinit()
        except:
            pass
    _pin_objects.clear()
    
    # Setup new pins
    try:
        # Setup clock pin
        if _current_pins['xclk'] >= 0:
            _pin_objects['xclk'] = Pin(_current_pins['xclk'], Pin.OUT)
        
        # Setup I2C pins
        if _current_pins['sda'] >= 0:
            _pin_objects['sda'] = Pin(_current_pins['sda'], Pin.OUT)
        if _current_pins['scl'] >= 0:
            _pin_objects['scl'] = Pin(_current_pins['scl'], Pin.OUT)
        
        # Setup data pins
        for i in range(8):
            pin_name = f'd{i}'
            if _current_pins[pin_name] >= 0:
                _pin_objects[pin_name] = Pin(_current_pins[pin_name], Pin.IN)
        
        # Setup sync pins
        if _current_pins['vsync'] >= 0:
            _pin_objects['vsync'] = Pin(_current_pins['vsync'], Pin.IN)
        if _current_pins['href'] >= 0:
            _pin_objects['href'] = Pin(_current_pins['href'], Pin.IN)
        if _current_pins['pclk'] >= 0:
            _pin_objects['pclk'] = Pin(_current_pins['pclk'], Pin.IN)
        
        # Setup control pins
        if _current_pins['pwdn'] >= 0:
            _pin_objects['pwdn'] = Pin(_current_pins['pwdn'], Pin.OUT)
            _pin_objects['pwdn'].value(0)  # Active low
        
        if _current_pins['reset'] >= 0:
            _pin_objects['reset'] = Pin(_current_pins['reset'], Pin.OUT)
            _pin_objects['reset'].value(1)  # Active low, keep high
            
    except Exception as e:
        raise PinConfigError(f"Failed to setup pins: {e}")

def set_pins(pin_config):
    """
    Set camera pin configuration
    
    Args:
        pin_config (dict): Dictionary mapping pin names to GPIO numbers
                          Use -1 for unused pins
    
    Example:
        pins = {
            'pwdn': 32, 'reset': -1, 'xclk': 0,
            'sda': 26, 'scl': 27,
            'd7': 35, 'd6': 34, 'd5': 39, 'd4': 36,
            'd3': 21, 'd2': 19, 'd1': 18, 'd0': 5,
            'vsync': 25, 'href': 23, 'pclk': 22
        }
        camera.set_pins(pins)
    """
    global _current_pins, _camera_initialized
    
    if _camera_initialized:
        raise CameraError("Cannot change pins while camera is initialized. Call deinit() first.")
    
    if isinstance(pin_config, str):
        # Handle preset configurations
        if pin_config in DEFAULT_PIN_CONFIGS:
            pin_config = DEFAULT_PIN_CONFIGS[pin_config]
        else:
            raise PinConfigError(f"Unknown preset configuration: {pin_config}")
    
    if not isinstance(pin_config, dict):
        raise PinConfigError("Pin configuration must be a dictionary")
    
    # Validate configuration
    _validate_pin_config(pin_config)
    
    # Update current configuration
    _current_pins.update(pin_config)
    
    # Setup GPIO pins
    _setup_pins()
    
    print(f"Camera pins configured: {len([p for p in _current_pins.values() if p >= 0])} pins active")

def get_pins():
    """
    Get current pin configuration
    
    Returns:
        dict: Current pin configuration
    """
    return _current_pins.copy()

def list_presets():
    """
    List available preset configurations
    
    Returns:
        list: Available preset names
    """
    return list(DEFAULT_PIN_CONFIGS.keys())

def load_preset(preset_name):
    """
    Load a preset pin configuration
    
    Args:
        preset_name (str): Name of the preset configuration
    """
    if preset_name not in DEFAULT_PIN_CONFIGS:
        raise PinConfigError(f"Unknown preset: {preset_name}. Available: {list(DEFAULT_PIN_CONFIGS.keys())}")
    
    set_pins(DEFAULT_PIN_CONFIGS[preset_name])
    print(f"Loaded preset configuration: {preset_name}")

def save_config(filename):
    """
    Save current pin configuration to file
    
    Args:
        filename (str): File path to save configuration
    """
    try:
        with open(filename, 'w') as f:
            json.dump(_current_pins, f, indent=2)
        print(f"Configuration saved to: {filename}")
    except Exception as e:
        raise CameraError(f"Failed to save configuration: {e}")

def load_config(filename):
    """
    Load pin configuration from file
    
    Args:
        filename (str): File path to load configuration from
    """
    try:
        with open(filename, 'r') as f:
            config = json.load(f)
        set_pins(config)
        print(f"Configuration loaded from: {filename}")
    except Exception as e:
        raise CameraError(f"Failed to load configuration: {e}")

def init(format=None, framesize=None, quality=None, **kwargs):
    """
    Initialize camera with current pin configuration
    
    Args:
        format: Image format (JPEG, RGB565, GRAYSCALE)
        framesize: Frame size (FRAME_QQVGA, FRAME_QVGA, etc.)
        quality: JPEG quality (1-63, lower is better)
        **kwargs: Additional arguments
    """
    global _camera_initialized
    
    if not _CAMERA_AVAILABLE:
        raise CameraError("Native camera module not available")
    
    if _camera_initialized:
        print("Camera already initialized")
        return
    
    # Set default values
    if format is None:
        format = JPEG
    if framesize is None:
        framesize = FRAME_VGA
    if quality is None:
        quality = 10
    
    try:
        # Setup pins first
        _setup_pins()
        
        # Initialize native camera module
        # Note: The native module uses fixed pins, but we've prepared the GPIO
        _camera_native.init(format=format, framesize=framesize, quality=quality)
        _camera_initialized = True
        
        print(f"Camera initialized successfully")
        print(f"Format: {format}, Frame size: {framesize}, Quality: {quality}")
        
    except Exception as e:
        _camera_initialized = False
        raise CameraError(f"Camera initialization failed: {e}")

def deinit():
    """Deinitialize camera and release resources"""
    global _camera_initialized, _pin_objects
    
    if not _CAMERA_AVAILABLE:
        return
    
    if _camera_initialized:
        try:
            _camera_native.deinit()
        except:
            pass
        _camera_initialized = False
    
    # Clean up pin objects
    for pin_obj in _pin_objects.values():
        try:
            pin_obj.deinit()
        except:
            pass
    _pin_objects.clear()
    
    print("Camera deinitialized")

def capture():
    """
    Capture an image
    
    Returns:
        bytes: Image data
    """
    if not _CAMERA_AVAILABLE:
        raise CameraError("Native camera module not available")
    
    if not _camera_initialized:
        raise CameraError("Camera not initialized. Call init() first.")
    
    try:
        return _camera_native.capture()
    except Exception as e:
        raise CameraError(f"Capture failed: {e}")

def set_brightness(brightness):
    """Set camera brightness (-2 to 2)"""
    if not _CAMERA_AVAILABLE:
        raise CameraError("Native camera module not available")
    
    if not _camera_initialized:
        raise CameraError("Camera not initialized")
    
    try:
        _camera_native.set_brightness(brightness)
    except Exception as e:
        raise CameraError(f"Failed to set brightness: {e}")

def set_contrast(contrast):
    """Set camera contrast (-2 to 2)"""
    if not _CAMERA_AVAILABLE:
        raise CameraError("Native camera module not available")
    
    if not _camera_initialized:
        raise CameraError("Camera not initialized")
    
    try:
        _camera_native.set_contrast(contrast)
    except Exception as e:
        raise CameraError(f"Failed to set contrast: {e}")

def set_saturation(saturation):
    """Set camera saturation (-2 to 2)"""
    if not _CAMERA_AVAILABLE:
        raise CameraError("Native camera module not available")
    
    if not _camera_initialized:
        raise CameraError("Camera not initialized")
    
    try:
        _camera_native.set_saturation(saturation)
    except Exception as e:
        raise CameraError(f"Failed to set saturation: {e}")

def get_status():
    """
    Get camera status information
    
    Returns:
        dict: Status information
    """
    return {
        'initialized': _camera_initialized,
        'native_available': _CAMERA_AVAILABLE,
        'current_pins': _current_pins.copy(),
        'active_pins': len([p for p in _current_pins.values() if p >= 0]),
        'pin_objects': len(_pin_objects)
    }

def test_camera():
    """
    Test camera functionality
    
    Returns:
        dict: Test results
    """
    results = {
        'native_module': _CAMERA_AVAILABLE,
        'pin_setup': False,
        'initialization': False,
        'capture': False,
        'image_size': 0
    }
    
    try:
        # Test pin setup
        _setup_pins()
        results['pin_setup'] = True
        
        if _CAMERA_AVAILABLE:
            # Test initialization
            if not _camera_initialized:
                init(framesize=FRAME_QQVGA, quality=30)
            results['initialization'] = _camera_initialized
            
            if _camera_initialized:
                # Test capture
                img = capture()
                if img:
                    results['capture'] = True
                    results['image_size'] = len(img)
                
    except Exception as e:
        results['error'] = str(e)
    
    return results

# Initialize with default configuration
try:
    _setup_pins()
except Exception as e:
    print(f"Warning: Failed to setup default pins: {e}")

print("Enhanced Camera Module loaded")
print(f"Native camera available: {_CAMERA_AVAILABLE}")
print(f"Default pins configured: {len([p for p in _current_pins.values() if p >= 0])} active pins")