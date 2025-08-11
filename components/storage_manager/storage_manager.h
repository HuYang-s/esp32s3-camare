#ifndef STORAGE_MANAGER_H
#define STORAGE_MANAGER_H

#include "esp_err.h"
#include "camera_driver.h"
#include <time.h>
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

#define MAX_HISTORY_IMAGES 3

typedef struct {
    char filename[64];
    time_t capture_time;
    bool has_ai_result;
    char ai_description[256];
} image_info_t;

/**
 * @brief 初始化存储管理器 (SPIFFS 和 互斥锁)
 */
esp_err_t storage_manager_init(void);

/**
 * @brief 保存图片到文件系统并添加到历史记录
 * @param fb 指向图像帧缓冲区的指针
 * @param output_filename 用于存储保存文件名的缓冲区
 * @param output_size 缓冲区大小
 * @return esp_err_t
 */
esp_err_t storage_manager_save_image(camera_fb_t *fb, char* output_filename, size_t output_size);

/**
 * @brief 根据文件名更新历史记录中的AI分析结果
 */
void storage_manager_update_ai_result(const char* filename, const char* ai_description);

/**
 * @brief 线程安全地获取图片历史记录
 * @param history_buffer 用于存储历史记录副本的缓冲区
 * @param buffer_size 缓冲区的容量 (应 >= MAX_HISTORY_IMAGES)
 * @return int 实际获取到的历史记录数量
 */
int storage_manager_get_history(image_info_t* history_buffer, int buffer_size);

/**
 * @brief 获取图片历史互斥锁的句柄
 * @note 仅供需要直接且长时间操作历史记录的特殊情况使用 (例如web服务器的API)
 *       通常应优先使用 storage_manager_get_history
 */
SemaphoreHandle_t storage_manager_get_history_mutex(void);


#endif // STORAGE_MANAGER_H