#include <string.h>
#include <sys/unistd.h>
#include <sys/stat.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "esp_spiffs.h"
#include "esp_log.h"

#include "storage_manager.h"

static const char *TAG = "storage_manager";

// 图片历史管理
static image_info_t image_history[MAX_HISTORY_IMAGES];
static int current_image_index = 0;
static int total_images = 0;
static SemaphoreHandle_t image_history_mutex;

// --- Private Functions ---

static void add_image_to_history(const char* filename, const char* ai_description)
{
    if (xSemaphoreTake(image_history_mutex, pdMS_TO_TICKS(1000)) == pdTRUE) {
        ESP_LOGI(TAG, "添加图片到历史: %s", filename);
        
        if (total_images >= MAX_HISTORY_IMAGES) {
            ESP_LOGI(TAG, "删除旧图片: %s", image_history[current_image_index].filename);
            unlink(image_history[current_image_index].filename);
        } else {
            total_images++;
        }
        
        strncpy(image_history[current_image_index].filename, filename, sizeof(image_history[current_image_index].filename) - 1);
        image_history[current_image_index].filename[sizeof(image_history[current_image_index].filename) - 1] = '\0';
        
        time(&image_history[current_image_index].capture_time);
        image_history[current_image_index].has_ai_result = (ai_description != NULL);
        
        if (ai_description) {
            strncpy(image_history[current_image_index].ai_description, ai_description, sizeof(image_history[current_image_index].ai_description) - 1);
            image_history[current_image_index].ai_description[sizeof(image_history[current_image_index].ai_description) - 1] = '\0';
        } else {
            strcpy(image_history[current_image_index].ai_description, "分析中...");
        }
        
        ESP_LOGI(TAG, "图片已添加到历史 (索引: %d, 总数: %d)", current_image_index, total_images);
        
        current_image_index = (current_image_index + 1) % MAX_HISTORY_IMAGES;
        
        xSemaphoreGive(image_history_mutex);
    }
}

// --- Public Functions ---

esp_err_t storage_manager_init(void)
{
    image_history_mutex = xSemaphoreCreateMutex();
    if (image_history_mutex == NULL) {
        ESP_LOGE(TAG, "Failed to create image history mutex");
        return ESP_FAIL;
    }

    esp_vfs_spiffs_conf_t conf = {
        .base_path = "/spiffs",
        .partition_label = NULL,
        .max_files = 5,
        .format_if_mount_failed = true
    };

    esp_err_t ret = esp_vfs_spiffs_register(&conf);
    if (ret != ESP_OK) {
        if (ret == ESP_FAIL) {
            ESP_LOGE(TAG, "Failed to mount or format filesystem");
        } else if (ret == ESP_ERR_NOT_FOUND) {
            ESP_LOGE(TAG, "Failed to find SPIFFS partition");
        } else {
            ESP_LOGE(TAG, "Failed to initialize SPIFFS (%s)", esp_err_to_name(ret));
        }
        return ret;
    }

    size_t total = 0, used = 0;
    ret = esp_spiffs_info(NULL, &total, &used);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to get SPIFFS partition information (%s)", esp_err_to_name(ret));
    } else {
        ESP_LOGI(TAG, "Partition size: total: %d, used: %d", total, used);
    }

    return ESP_OK;
}

esp_err_t storage_manager_save_image(camera_fb_t *fb, char* output_filename, size_t output_size)
{
    char filename[64];
    time_t now;
    time(&now);
    
    snprintf(filename, sizeof(filename), "/spiffs/img_%lld.jpg", (long long)now);
    
    FILE *f = fopen(filename, "w");
    if (f == NULL) {
        ESP_LOGE(TAG, "Failed to open file for writing");
        return ESP_FAIL;
    }
    
    size_t written = fwrite(fb->buf, 1, fb->len, f);
    fclose(f);
    
    if (written != fb->len) {
        ESP_LOGE(TAG, "Failed to write complete image");
        return ESP_FAIL;
    }
    
    ESP_LOGI(TAG, "Image saved to %s (%d bytes)", filename, fb->len);
    
    if (output_filename && output_size > 0) {
        strncpy(output_filename, filename, output_size - 1);
        output_filename[output_size - 1] = '\0';
    }
    
    add_image_to_history(filename, NULL);
    
    return ESP_OK;
}

void storage_manager_update_ai_result(const char* filename, const char* ai_description)
{
    if (xSemaphoreTake(image_history_mutex, pdMS_TO_TICKS(1000)) == pdTRUE) {
        for (int i = 0; i < total_images; i++) {
            if (strstr(image_history[i].filename, filename) != NULL) {
                image_history[i].has_ai_result = true;
                strncpy(image_history[i].ai_description, ai_description, sizeof(image_history[i].ai_description) - 1);
                image_history[i].ai_description[sizeof(image_history[i].ai_description) - 1] = '\0';
                ESP_LOGI(TAG, "更新图片AI结果: %s", filename);
                break;
            }
        }
        xSemaphoreGive(image_history_mutex);
    }
}

int storage_manager_get_history(image_info_t* history_buffer, int buffer_size)
{
    int count = 0;
    if (xSemaphoreTake(image_history_mutex, pdMS_TO_TICKS(1000)) == pdTRUE) {
        count = total_images < buffer_size ? total_images : buffer_size;
        for (int i = 0; i < count; i++) {
            memcpy(&history_buffer[i], &image_history[i], sizeof(image_info_t));
        }
        xSemaphoreGive(image_history_mutex);
    }
    return count;
}

SemaphoreHandle_t storage_manager_get_history_mutex(void)
{
    return image_history_mutex;
}
