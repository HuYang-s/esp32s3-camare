#pragma once

#include "esp_camera.h"
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

esp_err_t camera_init(void);
esp_err_t camera_capture(camera_fb_t** fb);
void camera_return_fb(camera_fb_t* fb);

#ifdef __cplusplus
}
#endif