/*
 * Enhanced ESP32-S3 Camera Module for MicroPython
 * Supports dynamic pin configuration and advanced camera features
 */

#include "py/runtime.h"
#include "py/mphal.h"
#include "py/obj.h"
#include "esp_camera.h"
#include "esp_log.h"
#include "esp_system.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "camera_enhanced";

// Camera configuration structure
static camera_config_t camera_config = {0};
static bool camera_initialized = false;

// Default pin configuration for ESP32-S3-CAM
static camera_pin_config_t default_pins = {
    .pin_pwdn = 32,
    .pin_reset = -1,
    .pin_xclk = 0,
    .pin_sccb_sda = 26,
    .pin_sccb_scl = 27,
    .pin_d7 = 35,
    .pin_d6 = 34,
    .pin_d5 = 39,
    .pin_d4 = 36,
    .pin_d3 = 21,
    .pin_d2 = 19,
    .pin_d1 = 18,
    .pin_d0 = 5,
    .pin_vsync = 25,
    .pin_href = 23,
    .pin_pclk = 22,
};

// Current pin configuration (can be modified)
static camera_pin_config_t current_pins;

// Initialize camera with default or custom pins
STATIC mp_obj_t camera_init(size_t n_args, const mp_obj_t *args, mp_map_t *kw_args) {
    enum {
        ARG_format, ARG_framesize, ARG_quality, ARG_pins
    };
    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_format, MP_ARG_KW_ONLY | MP_ARG_INT, {.u_int = PIXFORMAT_JPEG} },
        { MP_QSTR_framesize, MP_ARG_KW_ONLY | MP_ARG_INT, {.u_int = FRAMESIZE_VGA} },
        { MP_QSTR_quality, MP_ARG_KW_ONLY | MP_ARG_INT, {.u_int = 10} },
        { MP_QSTR_pins, MP_ARG_KW_ONLY | MP_ARG_OBJ, {.u_obj = mp_const_none} },
    };

    mp_arg_val_t parsed_args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all(n_args, args, kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, parsed_args);

    if (camera_initialized) {
        esp_camera_deinit();
        camera_initialized = false;
    }

    // Use default pins initially
    current_pins = default_pins;

    // Parse custom pin configuration if provided
    if (parsed_args[ARG_pins].u_obj != mp_const_none) {
        mp_obj_t *pin_items;
        size_t pin_len;
        mp_obj_get_array(parsed_args[ARG_pins].u_obj, &pin_len, &pin_items);
        
        if (pin_len >= 16) {
            current_pins.pin_pwdn = mp_obj_get_int(pin_items[0]);
            current_pins.pin_reset = mp_obj_get_int(pin_items[1]);
            current_pins.pin_xclk = mp_obj_get_int(pin_items[2]);
            current_pins.pin_sccb_sda = mp_obj_get_int(pin_items[3]);
            current_pins.pin_sccb_scl = mp_obj_get_int(pin_items[4]);
            current_pins.pin_d7 = mp_obj_get_int(pin_items[5]);
            current_pins.pin_d6 = mp_obj_get_int(pin_items[6]);
            current_pins.pin_d5 = mp_obj_get_int(pin_items[7]);
            current_pins.pin_d4 = mp_obj_get_int(pin_items[8]);
            current_pins.pin_d3 = mp_obj_get_int(pin_items[9]);
            current_pins.pin_d2 = mp_obj_get_int(pin_items[10]);
            current_pins.pin_d1 = mp_obj_get_int(pin_items[11]);
            current_pins.pin_d0 = mp_obj_get_int(pin_items[12]);
            current_pins.pin_vsync = mp_obj_get_int(pin_items[13]);
            current_pins.pin_href = mp_obj_get_int(pin_items[14]);
            current_pins.pin_pclk = mp_obj_get_int(pin_items[15]);
        }
    }

    // Configure camera
    camera_config.pin_config = current_pins;
    camera_config.ledc_channel = LEDC_CHANNEL_0;
    camera_config.ledc_timer = LEDC_TIMER_0;
    camera_config.pixel_format = parsed_args[ARG_format].u_int;
    camera_config.frame_size = parsed_args[ARG_framesize].u_int;
    camera_config.jpeg_quality = parsed_args[ARG_quality].u_int;
    camera_config.fb_count = 1;
    camera_config.grab_mode = CAMERA_GRAB_WHEN_EMPTY;

    // Initialize camera
    esp_err_t err = esp_camera_init(&camera_config);
    if (err != ESP_OK) {
        mp_raise_OSError(MP_EIO);
    }

    camera_initialized = true;
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(camera_init_obj, 0, camera_init);

// Set individual pin configuration
STATIC mp_obj_t camera_set_pins(mp_obj_t pin_dict) {
    if (!mp_obj_is_type(pin_dict, &mp_type_dict)) {
        mp_raise_TypeError(MP_ERROR_TEXT("pins must be a dictionary"));
    }

    mp_obj_dict_t *dict = MP_OBJ_TO_PTR(pin_dict);
    
    // Update pins based on dictionary keys
    mp_obj_t key, value;
    
    if (mp_obj_dict_get(pin_dict, MP_OBJ_NEW_QSTR(MP_QSTR_pwdn), &value)) {
        current_pins.pin_pwdn = mp_obj_get_int(value);
    }
    if (mp_obj_dict_get(pin_dict, MP_OBJ_NEW_QSTR(MP_QSTR_reset), &value)) {
        current_pins.pin_reset = mp_obj_get_int(value);
    }
    if (mp_obj_dict_get(pin_dict, MP_OBJ_NEW_QSTR(MP_QSTR_xclk), &value)) {
        current_pins.pin_xclk = mp_obj_get_int(value);
    }
    if (mp_obj_dict_get(pin_dict, MP_OBJ_NEW_QSTR(MP_QSTR_sda), &value)) {
        current_pins.pin_sccb_sda = mp_obj_get_int(value);
    }
    if (mp_obj_dict_get(pin_dict, MP_OBJ_NEW_QSTR(MP_QSTR_scl), &value)) {
        current_pins.pin_sccb_scl = mp_obj_get_int(value);
    }
    if (mp_obj_dict_get(pin_dict, MP_OBJ_NEW_QSTR(MP_QSTR_d7), &value)) {
        current_pins.pin_d7 = mp_obj_get_int(value);
    }
    if (mp_obj_dict_get(pin_dict, MP_OBJ_NEW_QSTR(MP_QSTR_d6), &value)) {
        current_pins.pin_d6 = mp_obj_get_int(value);
    }
    if (mp_obj_dict_get(pin_dict, MP_OBJ_NEW_QSTR(MP_QSTR_d5), &value)) {
        current_pins.pin_d5 = mp_obj_get_int(value);
    }
    if (mp_obj_dict_get(pin_dict, MP_OBJ_NEW_QSTR(MP_QSTR_d4), &value)) {
        current_pins.pin_d4 = mp_obj_get_int(value);
    }
    if (mp_obj_dict_get(pin_dict, MP_OBJ_NEW_QSTR(MP_QSTR_d3), &value)) {
        current_pins.pin_d3 = mp_obj_get_int(value);
    }
    if (mp_obj_dict_get(pin_dict, MP_OBJ_NEW_QSTR(MP_QSTR_d2), &value)) {
        current_pins.pin_d2 = mp_obj_get_int(value);
    }
    if (mp_obj_dict_get(pin_dict, MP_OBJ_NEW_QSTR(MP_QSTR_d1), &value)) {
        current_pins.pin_d1 = mp_obj_get_int(value);
    }
    if (mp_obj_dict_get(pin_dict, MP_OBJ_NEW_QSTR(MP_QSTR_d0), &value)) {
        current_pins.pin_d0 = mp_obj_get_int(value);
    }
    if (mp_obj_dict_get(pin_dict, MP_OBJ_NEW_QSTR(MP_QSTR_vsync), &value)) {
        current_pins.pin_vsync = mp_obj_get_int(value);
    }
    if (mp_obj_dict_get(pin_dict, MP_OBJ_NEW_QSTR(MP_QSTR_href), &value)) {
        current_pins.pin_href = mp_obj_get_int(value);
    }
    if (mp_obj_dict_get(pin_dict, MP_OBJ_NEW_QSTR(MP_QSTR_pclk), &value)) {
        current_pins.pin_pclk = mp_obj_get_int(value);
    }

    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(camera_set_pins_obj, camera_set_pins);

// Get current pin configuration
STATIC mp_obj_t camera_get_pins(void) {
    mp_obj_t dict = mp_obj_new_dict(16);
    
    mp_obj_dict_store(dict, MP_OBJ_NEW_QSTR(MP_QSTR_pwdn), mp_obj_new_int(current_pins.pin_pwdn));
    mp_obj_dict_store(dict, MP_OBJ_NEW_QSTR(MP_QSTR_reset), mp_obj_new_int(current_pins.pin_reset));
    mp_obj_dict_store(dict, MP_OBJ_NEW_QSTR(MP_QSTR_xclk), mp_obj_new_int(current_pins.pin_xclk));
    mp_obj_dict_store(dict, MP_OBJ_NEW_QSTR(MP_QSTR_sda), mp_obj_new_int(current_pins.pin_sccb_sda));
    mp_obj_dict_store(dict, MP_OBJ_NEW_QSTR(MP_QSTR_scl), mp_obj_new_int(current_pins.pin_sccb_scl));
    mp_obj_dict_store(dict, MP_OBJ_NEW_QSTR(MP_QSTR_d7), mp_obj_new_int(current_pins.pin_d7));
    mp_obj_dict_store(dict, MP_OBJ_NEW_QSTR(MP_QSTR_d6), mp_obj_new_int(current_pins.pin_d6));
    mp_obj_dict_store(dict, MP_OBJ_NEW_QSTR(MP_QSTR_d5), mp_obj_new_int(current_pins.pin_d5));
    mp_obj_dict_store(dict, MP_OBJ_NEW_QSTR(MP_QSTR_d4), mp_obj_new_int(current_pins.pin_d4));
    mp_obj_dict_store(dict, MP_OBJ_NEW_QSTR(MP_QSTR_d3), mp_obj_new_int(current_pins.pin_d3));
    mp_obj_dict_store(dict, MP_OBJ_NEW_QSTR(MP_QSTR_d2), mp_obj_new_int(current_pins.pin_d2));
    mp_obj_dict_store(dict, MP_OBJ_NEW_QSTR(MP_QSTR_d1), mp_obj_new_int(current_pins.pin_d1));
    mp_obj_dict_store(dict, MP_OBJ_NEW_QSTR(MP_QSTR_d0), mp_obj_new_int(current_pins.pin_d0));
    mp_obj_dict_store(dict, MP_OBJ_NEW_QSTR(MP_QSTR_vsync), mp_obj_new_int(current_pins.pin_vsync));
    mp_obj_dict_store(dict, MP_OBJ_NEW_QSTR(MP_QSTR_href), mp_obj_new_int(current_pins.pin_href));
    mp_obj_dict_store(dict, MP_OBJ_NEW_QSTR(MP_QSTR_pclk), mp_obj_new_int(current_pins.pin_pclk));
    
    return dict;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_0(camera_get_pins_obj, camera_get_pins);

// Capture image
STATIC mp_obj_t camera_capture(void) {
    if (!camera_initialized) {
        mp_raise_RuntimeError(MP_ERROR_TEXT("Camera not initialized"));
    }

    camera_fb_t *fb = esp_camera_fb_get();
    if (!fb) {
        mp_raise_RuntimeError(MP_ERROR_TEXT("Camera capture failed"));
    }

    mp_obj_t image_data = mp_obj_new_bytes(fb->buf, fb->len);
    esp_camera_fb_return(fb);
    
    return image_data;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_0(camera_capture_obj, camera_capture);

// Deinitialize camera
STATIC mp_obj_t camera_deinit(void) {
    if (camera_initialized) {
        esp_camera_deinit();
        camera_initialized = false;
    }
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_0(camera_deinit_obj, camera_deinit);

// Camera sensor settings
STATIC mp_obj_t camera_set_brightness(mp_obj_t brightness_obj) {
    if (!camera_initialized) {
        mp_raise_RuntimeError(MP_ERROR_TEXT("Camera not initialized"));
    }
    
    int brightness = mp_obj_get_int(brightness_obj);
    sensor_t *s = esp_camera_sensor_get();
    s->set_brightness(s, brightness);
    
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(camera_set_brightness_obj, camera_set_brightness);

STATIC mp_obj_t camera_set_contrast(mp_obj_t contrast_obj) {
    if (!camera_initialized) {
        mp_raise_RuntimeError(MP_ERROR_TEXT("Camera not initialized"));
    }
    
    int contrast = mp_obj_get_int(contrast_obj);
    sensor_t *s = esp_camera_sensor_get();
    s->set_contrast(s, contrast);
    
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(camera_set_contrast_obj, camera_set_contrast);

STATIC mp_obj_t camera_set_saturation(mp_obj_t saturation_obj) {
    if (!camera_initialized) {
        mp_raise_RuntimeError(MP_ERROR_TEXT("Camera not initialized"));
    }
    
    int saturation = mp_obj_get_int(saturation_obj);
    sensor_t *s = esp_camera_sensor_get();
    s->set_saturation(s, saturation);
    
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(camera_set_saturation_obj, camera_set_saturation);

// Module globals table
STATIC const mp_rom_map_elem_t camera_module_globals_table[] = {
    { MP_ROM_QSTR(MP_QSTR___name__), MP_ROM_QSTR(MP_QSTR_camera) },
    
    // Functions
    { MP_ROM_QSTR(MP_QSTR_init), MP_ROM_PTR(&camera_init_obj) },
    { MP_ROM_QSTR(MP_QSTR_deinit), MP_ROM_PTR(&camera_deinit_obj) },
    { MP_ROM_QSTR(MP_QSTR_capture), MP_ROM_PTR(&camera_capture_obj) },
    { MP_ROM_QSTR(MP_QSTR_set_pins), MP_ROM_PTR(&camera_set_pins_obj) },
    { MP_ROM_QSTR(MP_QSTR_get_pins), MP_ROM_PTR(&camera_get_pins_obj) },
    { MP_ROM_QSTR(MP_QSTR_set_brightness), MP_ROM_PTR(&camera_set_brightness_obj) },
    { MP_ROM_QSTR(MP_QSTR_set_contrast), MP_ROM_PTR(&camera_set_contrast_obj) },
    { MP_ROM_QSTR(MP_QSTR_set_saturation), MP_ROM_PTR(&camera_set_saturation_obj) },
    
    // Format constants
    { MP_ROM_QSTR(MP_QSTR_JPEG), MP_ROM_INT(PIXFORMAT_JPEG) },
    { MP_ROM_QSTR(MP_QSTR_RGB565), MP_ROM_INT(PIXFORMAT_RGB565) },
    { MP_ROM_QSTR(MP_QSTR_GRAYSCALE), MP_ROM_INT(PIXFORMAT_GRAYSCALE) },
    
    // Frame size constants
    { MP_ROM_QSTR(MP_QSTR_FRAME_QQVGA), MP_ROM_INT(FRAMESIZE_QQVGA) },   // 160x120
    { MP_ROM_QSTR(MP_QSTR_FRAME_QVGA), MP_ROM_INT(FRAMESIZE_QVGA) },     // 320x240
    { MP_ROM_QSTR(MP_QSTR_FRAME_VGA), MP_ROM_INT(FRAMESIZE_VGA) },       // 640x480
    { MP_ROM_QSTR(MP_QSTR_FRAME_SVGA), MP_ROM_INT(FRAMESIZE_SVGA) },     // 800x600
    { MP_ROM_QSTR(MP_QSTR_FRAME_XGA), MP_ROM_INT(FRAMESIZE_XGA) },       // 1024x768
    { MP_ROM_QSTR(MP_QSTR_FRAME_SXGA), MP_ROM_INT(FRAMESIZE_SXGA) },     // 1280x1024
    { MP_ROM_QSTR(MP_QSTR_FRAME_UXGA), MP_ROM_INT(FRAMESIZE_UXGA) },     // 1600x1200
};
STATIC MP_DEFINE_CONST_DICT(camera_module_globals, camera_module_globals_table);

// Module definition
const mp_obj_module_t camera_user_cmodule = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t *)&camera_module_globals,
};

// Register the module
MP_REGISTER_MODULE(MP_QSTR_camera, camera_user_cmodule);