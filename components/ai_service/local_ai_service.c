#include <stdio.h>
#include <string.h>
#include <time.h>
#include <math.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "esp_log.h"

#include "local_ai_service.h"
#include "storage_manager.h"

static const char *TAG = "local_ai";

// 全局AI任务状态
static ai_task_t current_task = {0};
static SemaphoreHandle_t task_mutex = NULL;

// COCO数据集常见类别名称（简化版）
static const char* class_names[] = {
    "person", "bicycle", "car", "motorcycle", "airplane", "bus", "train", "truck", "boat",
    "traffic light", "fire hydrant", "stop sign", "parking meter", "bench", "bird", "cat",
    "dog", "horse", "sheep", "cow", "elephant", "bear", "zebra", "giraffe", "backpack",
    "umbrella", "handbag", "tie", "suitcase", "frisbee", "skis", "snowboard", "sports ball",
    "kite", "baseball bat", "baseball glove", "skateboard", "surfboard", "tennis racket",
    "bottle", "wine glass", "cup", "fork", "knife", "spoon", "bowl", "banana", "apple",
    "sandwich", "orange", "broccoli", "carrot", "hot dog", "pizza", "donut", "cake",
    "chair", "couch", "potted plant", "bed", "dining table", "toilet", "tv", "laptop",
    "mouse", "remote", "keyboard", "cell phone", "microwave", "oven", "toaster", "sink",
    "refrigerator", "book", "clock", "vase", "scissors", "teddy bear", "hair drier", "toothbrush"
};

#define NUM_CLASSES (sizeof(class_names) / sizeof(class_names[0]))

esp_err_t local_ai_service_init(void)
{
    ESP_LOGI(TAG, "初始化本地AI服务...");
    
    if (task_mutex == NULL) {
        task_mutex = xSemaphoreCreateMutex();
        if (task_mutex == NULL) {
            ESP_LOGE(TAG, "创建任务互斥锁失败");
            return ESP_FAIL;
        }
    }
    
    // 初始化任务状态
    if (xSemaphoreTake(task_mutex, pdMS_TO_TICKS(1000)) == pdTRUE) {
        current_task.status = AI_TASK_IDLE;
        current_task.result_count = 0;
        strcpy(current_task.status_message, "本地AI服务已就绪");
        xSemaphoreGive(task_mutex);
    }
    
    ESP_LOGI(TAG, "本地AI服务初始化完成");
    ESP_LOGI(TAG, "支持 %d 种物体类别", NUM_CLASSES);
    
    return ESP_OK;
}

// 简化的物体检测算法（基于颜色和形状特征）
static int detect_objects_by_features(camera_fb_t *fb, detection_result_t *results, int max_results)
{
    if (!fb || !fb->buf || fb->len == 0) {
        ESP_LOGE(TAG, "无效的图像数据");
        return -1;
    }
    
    ESP_LOGI(TAG, "开始基于特征的物体检测，图像大小: %dx%d", fb->width, fb->height);
    
    int detection_count = 0;
    uint8_t *img_data = fb->buf;
    int img_width = fb->width;
    int img_height = fb->height;
    
    // 简化的检测算法：基于颜色块和边缘检测
    // 这里实现一个基础的物体检测逻辑
    
    // 检测1: 寻找红色物体（如苹果、消防栓等）
    if (detection_count < max_results) {
        int red_pixels = 0;
        int total_pixels = img_width * img_height;
        
        // 简化的红色检测（假设JPEG格式）
        for (int i = 0; i < fb->len - 2; i += 3) {
            uint8_t r = img_data[i];
            uint8_t g = img_data[i + 1]; 
            uint8_t b = img_data[i + 2];
            
            // 检测红色像素 (R > 150, G < 100, B < 100)
            if (r > 150 && g < 100 && b < 100) {
                red_pixels++;
            }
        }
        
        float red_ratio = (float)red_pixels / (total_pixels / 3);
        if (red_ratio > 0.05) { // 如果红色像素超过5%
            results[detection_count].x = 0.3f;
            results[detection_count].y = 0.3f;
            results[detection_count].width = 0.4f;
            results[detection_count].height = 0.4f;
            results[detection_count].confidence = red_ratio * 2.0f; // 简化的置信度
            if (results[detection_count].confidence > 1.0f) results[detection_count].confidence = 1.0f;
            results[detection_count].class_id = 47; // apple
            strcpy(results[detection_count].class_name, "apple");
            detection_count++;
            ESP_LOGI(TAG, "检测到红色物体 (可能是苹果), 置信度: %.2f", results[detection_count-1].confidence);
        }
    }
    
    // 检测2: 寻找绿色物体（如植物）
    if (detection_count < max_results) {
        int green_pixels = 0;
        int total_pixels = img_width * img_height;
        
        for (int i = 0; i < fb->len - 2; i += 3) {
            uint8_t r = img_data[i];
            uint8_t g = img_data[i + 1];
            uint8_t b = img_data[i + 2];
            
            // 检测绿色像素 (G > 120, R < 100, B < 100)
            if (g > 120 && r < 100 && b < 100) {
                green_pixels++;
            }
        }
        
        float green_ratio = (float)green_pixels / (total_pixels / 3);
        if (green_ratio > 0.08) { // 如果绿色像素超过8%
            results[detection_count].x = 0.1f;
            results[detection_count].y = 0.5f;
            results[detection_count].width = 0.3f;
            results[detection_count].height = 0.3f;
            results[detection_count].confidence = green_ratio * 1.5f;
            if (results[detection_count].confidence > 1.0f) results[detection_count].confidence = 1.0f;
            results[detection_count].class_id = 58; // potted plant
            strcpy(results[detection_count].class_name, "potted plant");
            detection_count++;
            ESP_LOGI(TAG, "检测到绿色物体 (可能是植物), 置信度: %.2f", results[detection_count-1].confidence);
        }
    }
    
    // 检测3: 基于亮度变化检测可能的物体边缘
    if (detection_count < max_results) {
        int edge_count = 0;
        
        // 简化的边缘检测
        for (int y = 1; y < img_height - 1; y++) {
            for (int x = 1; x < img_width - 1; x++) {
                // 简化的梯度计算（假设灰度图像）
                int idx = y * img_width + x;
                if (idx < fb->len - img_width - 1) {
                    int pixel = img_data[idx];
                    int right = img_data[idx + 1];
                    int down = img_data[idx + img_width];
                    
                    int gradient = abs(pixel - right) + abs(pixel - down);
                    if (gradient > 30) { // 边缘阈值
                        edge_count++;
                    }
                }
            }
        }
        
        float edge_density = (float)edge_count / (img_width * img_height);
        if (edge_density > 0.1) { // 如果边缘密度较高
            results[detection_count].x = 0.2f;
            results[detection_count].y = 0.2f;
            results[detection_count].width = 0.6f;
            results[detection_count].height = 0.6f;
            results[detection_count].confidence = edge_density * 3.0f;
            if (results[detection_count].confidence > 1.0f) results[detection_count].confidence = 1.0f;
            results[detection_count].class_id = 0; // person (通用物体)
            strcpy(results[detection_count].class_name, "object");
            detection_count++;
            ESP_LOGI(TAG, "检测到边缘丰富区域 (可能是物体), 置信度: %.2f", results[detection_count-1].confidence);
        }
    }
    
    ESP_LOGI(TAG, "检测完成，找到 %d 个物体", detection_count);
    return detection_count;
}

int local_ai_detect_objects(camera_fb_t *fb, detection_result_t *results, int max_results)
{
    if (!fb || !results || max_results <= 0) {
        ESP_LOGE(TAG, "无效的检测参数");
        return -1;
    }
    
    ESP_LOGI(TAG, "开始本地物体检测...");
    
    // 使用基于特征的检测算法
    int detection_count = detect_objects_by_features(fb, results, max_results);
    
    if (detection_count > 0) {
        ESP_LOGI(TAG, "本地AI检测到 %d 个物体", detection_count);
        for (int i = 0; i < detection_count; i++) {
            ESP_LOGI(TAG, "物体 %d: %s (置信度: %.2f, 位置: %.2f,%.2f,%.2f,%.2f)", 
                i + 1, results[i].class_name, results[i].confidence,
                results[i].x, results[i].y, results[i].width, results[i].height);
        }
    } else {
        ESP_LOGI(TAG, "未检测到任何物体");
    }
    
    return detection_count;
}

esp_err_t local_ai_start_task(const char *target_object, int timeout_seconds)
{
    if (!target_object || strlen(target_object) == 0) {
        ESP_LOGE(TAG, "目标物体名称无效");
        return ESP_FAIL;
    }
    
    if (xSemaphoreTake(task_mutex, pdMS_TO_TICKS(1000)) != pdTRUE) {
        ESP_LOGE(TAG, "无法获取任务锁");
        return ESP_FAIL;
    }
    
    // 检查当前任务状态
    if (current_task.status == AI_TASK_SEARCHING) {
        ESP_LOGW(TAG, "已有任务在进行中，停止当前任务");
    }
    
    // 初始化新任务
    current_task.status = AI_TASK_SEARCHING;
    strncpy(current_task.target_object, target_object, sizeof(current_task.target_object) - 1);
    current_task.target_object[sizeof(current_task.target_object) - 1] = '\0';
    current_task.timeout_seconds = timeout_seconds;
    current_task.start_time = time(NULL);
    current_task.result_count = 0;
    
    snprintf(current_task.status_message, sizeof(current_task.status_message),
        "正在搜索: %s (超时: %d秒)", target_object, timeout_seconds);
    
    xSemaphoreGive(task_mutex);
    
    ESP_LOGI(TAG, "AI任务已启动: 搜索 '%s'，超时 %d 秒", target_object, timeout_seconds);
    return ESP_OK;
}

esp_err_t local_ai_stop_task(void)
{
    if (xSemaphoreTake(task_mutex, pdMS_TO_TICKS(1000)) != pdTRUE) {
        return ESP_FAIL;
    }
    
    current_task.status = AI_TASK_IDLE;
    strcpy(current_task.status_message, "任务已停止");
    
    xSemaphoreGive(task_mutex);
    
    ESP_LOGI(TAG, "AI任务已停止");
    return ESP_OK;
}

const ai_task_t* local_ai_get_task_status(void)
{
    return &current_task;
}

esp_err_t local_ai_process_task(camera_fb_t *fb)
{
    if (xSemaphoreTake(task_mutex, pdMS_TO_TICKS(100)) != pdTRUE) {
        return ESP_FAIL;
    }
    
    // 检查任务状态
    if (current_task.status != AI_TASK_SEARCHING) {
        xSemaphoreGive(task_mutex);
        return ESP_OK; // 没有活动任务
    }
    
    // 检查超时
    time_t current_time = time(NULL);
    if (current_time - current_task.start_time >= current_task.timeout_seconds) {
        current_task.status = AI_TASK_FAILED_TIMEOUT;
        strcpy(current_task.status_message, "任务超时");
        ESP_LOGW(TAG, "AI任务超时");
        xSemaphoreGive(task_mutex);
        return ESP_OK;
    }
    
    xSemaphoreGive(task_mutex);
    
    // 执行物体检测
    detection_result_t temp_results[10];
    int detection_count = local_ai_detect_objects(fb, temp_results, 10);
    
    if (detection_count > 0) {
        if (xSemaphoreTake(task_mutex, pdMS_TO_TICKS(1000)) == pdTRUE) {
            // 检查是否找到目标物体
            bool found_target = false;
            for (int i = 0; i < detection_count; i++) {
                // 简单的字符串匹配（可以改进为更智能的匹配）
                if (strstr(temp_results[i].class_name, current_task.target_object) != NULL ||
                    strstr(current_task.target_object, temp_results[i].class_name) != NULL) {
                    found_target = true;
                    break;
                }
            }
            
            // 保存检测结果
            current_task.result_count = (detection_count > 10) ? 10 : detection_count;
            memcpy(current_task.results, temp_results, current_task.result_count * sizeof(detection_result_t));
            
            if (found_target) {
                current_task.status = AI_TASK_COMPLETED;
                snprintf(current_task.status_message, sizeof(current_task.status_message),
                    "找到目标物体: %s", current_task.target_object);
                ESP_LOGI(TAG, "AI任务完成: 找到目标物体 '%s'", current_task.target_object);
            } else {
                snprintf(current_task.status_message, sizeof(current_task.status_message),
                    "搜索中... 已检测到 %d 个物体", detection_count);
            }
            
            xSemaphoreGive(task_mutex);
        }
    }
    
    return ESP_OK;
}

esp_err_t local_ai_draw_detections(camera_fb_t *fb, const detection_result_t *results, int result_count)
{
    // 注意：实际的图像绘制需要根据图像格式实现
    // 这里只是记录检测结果用于调试
    
    ESP_LOGI(TAG, "绘制 %d 个检测结果:", result_count);
    for (int i = 0; i < result_count; i++) {
        ESP_LOGI(TAG, "检测框 %d: %s (%.2f) at (%.2f,%.2f,%.2f,%.2f)", 
            i + 1, results[i].class_name, results[i].confidence,
            results[i].x, results[i].y, results[i].width, results[i].height);
    }
    
    return ESP_OK;
}

const char* local_ai_get_class_name(int class_id)
{
    if (class_id >= 0 && class_id < NUM_CLASSES) {
        return class_names[class_id];
    }
    return "unknown";
}