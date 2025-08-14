#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "esp_system.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "camera_driver.h"

#include "wifi_manager.h"
#include "time_service.h"
#include "storage_manager.h"
#include "ai_service.h"
#include "web_server.h"
#include "motor_driver.h"
#include "local_ai_service.h"

static const char *TAG = "main";

// WiFi AP模式的密码，仅用于启动时打印
#define AP_PASS "12345678"

static SemaphoreHandle_t camera_mutex;

// AI模式选择: false = 普通分析, true = 自动驾驶
static bool ai_auto_drive_mode = false;
static int analysis_counter = 0;

// 电机测试任务
static void motor_test_task(void *pvParameters)
{
    ESP_LOGI(TAG, "开始电机测试任务");
    
    // 等待5秒让系统稳定
    vTaskDelay(pdMS_TO_TICKS(5000));
    
    // 执行电机测试序列
    motor_test_sequence();
    
    ESP_LOGI(TAG, "电机测试完成，删除任务");
    vTaskDelete(NULL);
}

static void capture_and_analyze_task(void *pvParameters)
{
    camera_fb_t *fb = NULL;
    
    while (1) {
        analysis_counter++;
        
        // 每10次分析切换一次模式（演示功能）
        if (analysis_counter % 10 == 5) {
            ai_auto_drive_mode = !ai_auto_drive_mode;
            printf("\n=== 切换AI模式 ===\n");
            printf("当前模式: %s\n", ai_auto_drive_mode ? "AI自动驾驶" : "AI普通分析");
            printf("==================\n\n");
        }
        
        printf("\n=== 开始拍照 (模式: %s) ===\n", ai_auto_drive_mode ? "AI自动驾驶" : "AI普通分析");
        ESP_LOGI(TAG, "正在拍照...");
        
        if (xSemaphoreTake(camera_mutex, pdMS_TO_TICKS(500)) == pdTRUE) {
            esp_err_t res = camera_capture(&fb);
            
            if (res == ESP_OK && fb != NULL) {
                printf("拍照完成，图片大小: %u 字节\n", (unsigned int)fb->len);
                
                char saved_filename[64];
                storage_manager_save_image(fb, saved_filename, sizeof(saved_filename));
                
                xSemaphoreGive(camera_mutex);
                
                // 处理本地AI任务
                esp_err_t local_ai_result = local_ai_process_task(fb);
                if (local_ai_result == ESP_OK) {
                    const ai_task_t* task_status = local_ai_get_task_status();
                    if (task_status->status != AI_TASK_IDLE) {
                        printf("本地AI任务状态: %s\n", task_status->status_message);
                    }
                }
                
                // 根据模式选择分析方式
                esp_err_t ai_result;
                if (ai_auto_drive_mode) {
                    printf("开始AI自动驾驶分析...\n");
                    ai_result = ai_service_auto_drive_analyze(fb, saved_filename);
                    if (ai_result == ESP_OK) {
                        printf("AI自动驾驶分析完成\n");
                    } else {
                        printf("AI自动驾驶分析失败\n");
                    }
                } else {
                    // 尝试本地AI检测
                    detection_result_t local_results[5];
                    int local_detection_count = local_ai_detect_objects(fb, local_results, 5);
                    
                    if (local_detection_count > 0) {
                        printf("本地AI检测到 %d 个物体:\n", local_detection_count);
                        char local_result_text[512] = "本地AI检测结果:\n";
                        for (int i = 0; i < local_detection_count; i++) {
                            printf("- %s (置信度: %.2f)\n", local_results[i].class_name, local_results[i].confidence);
                            char item_text[64];
                            snprintf(item_text, sizeof(item_text), "- %s (置信度: %.2f)\n", 
                                local_results[i].class_name, local_results[i].confidence);
                            strcat(local_result_text, item_text);
                        }
                        storage_manager_update_ai_result(saved_filename, local_result_text);
                        ai_result = ESP_OK;
                    } else {
                        // 如果本地AI没有检测到物体，则使用云端AI
                        ai_result = ai_service_analyze_image(fb, saved_filename);
                        if (ai_result == ESP_OK) {
                            printf("云端AI分析完成\n");
                        } else {
                            int failure_count = ai_service_get_socket_failure_count();
                            printf("云端AI分析失败 (失败次数: %d)\n", failure_count);
                            if (failure_count >= 3) {
                                printf("连续AI分析失败，等待5秒后重试...\n");
                                vTaskDelay(pdMS_TO_TICKS(5000));
                            }
                        }
                    }
                }
            
                camera_return_fb(fb);
                printf("AI分析完成，立即进行下次拍照...\n\n");
            } else {
                ESP_LOGE(TAG, "Camera capture failed");
                xSemaphoreGive(camera_mutex);
                printf("拍照失败，重试中...\n");
                vTaskDelay(pdMS_TO_TICKS(1000));
            }
        } else {
            ESP_LOGW(TAG, "无法获取摄像头访问权限");
            printf("无法获取摄像头访问权限，重试中...\n");
            vTaskDelay(pdMS_TO_TICKS(500));
        }
        
        if (ai_service_get_socket_failure_count() >= 5) {
            printf("连续AI分析失败过多，暂停3秒...\n");
            vTaskDelay(pdMS_TO_TICKS(3000));
        } else {
            // 自动驾驶模式稍微慢一些，避免过于频繁的控制
            int delay_ms = ai_auto_drive_mode ? 3000 : 1000;
            vTaskDelay(pdMS_TO_TICKS(delay_ms));
        }
    }
}

void app_main(void)
{
    ESP_LOGI(TAG, "Starting application - Free heap: %u bytes", (unsigned int)esp_get_free_heap_size());
    
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    camera_mutex = xSemaphoreCreateMutex();
    if (camera_mutex == NULL) {
        ESP_LOGE(TAG, "Failed to create camera mutex");
        return;
    }
    
    ESP_ERROR_CHECK(storage_manager_init());

    ESP_LOGI(TAG, "初始化WiFi AP+STA模式...");
    wifi_manager_init_ap_sta();
    
    if (wifi_manager_wait_for_connect(pdMS_TO_TICKS(30000)) == ESP_OK) {
        ESP_LOGI(TAG, "WiFi连接成功");
        printf("WiFi连接成功，开始初始化服务...\n");
        time_service_init();
    } else {
        ESP_LOGE(TAG, "WiFi连接超时");
        printf("WiFi连接失败，请检查网络配置\n");
        // 继续执行，AP模式可能仍然可用
    }

    ESP_ERROR_CHECK(camera_init());
    printf("摄像头初始化完成\n");

    // 初始化AI服务
    ESP_LOGI(TAG, "初始化AI服务...");
    esp_err_t ai_ret = ai_service_init();
    if (ai_ret == ESP_OK) {
        printf("AI服务初始化成功\n");
    } else {
        ESP_LOGE(TAG, "AI服务初始化失败");
        printf("AI服务初始化失败\n");
    }

    ESP_LOGI(TAG, "初始化L298N电机驱动...");
    esp_err_t motor_ret = motor_driver_init();
    if (motor_ret == ESP_OK) {
        printf("L298N电机驱动初始化成功\n");
        printf("GPIO引脚映射: IN1=GPIO%d, IN2=GPIO%d, IN3=GPIO%d, IN4=GPIO%d\n", 
               3, 1, 2, 42);
    } else {
        ESP_LOGE(TAG, "L298N电机驱动初始化失败");
        printf("L298N电机驱动初始化失败\n");
    }

    web_server_start();

    xTaskCreate(capture_and_analyze_task, "capture_analyze_task", 12288, NULL, 5, NULL);

    // 如果电机驱动初始化成功，启动电机测试任务
    if (motor_ret == ESP_OK) {
        xTaskCreate(motor_test_task, "motor_test_task", 4096, NULL, 3, NULL);
        printf("电机测试任务已启动，将在5秒后开始测试\n");
    }

    printf("\n系统启动完成，开始自动拍照和AI分析\n");
    printf("网络模式：AP+STA双模式\n");
    
    char sta_ip[16];
    wifi_manager_get_sta_ip(sta_ip, sizeof(sta_ip));
    printf("STA IP: %s\n", sta_ip);

    char ap_ip[16];
    wifi_manager_get_ap_ip(ap_ip, sizeof(ap_ip));
    printf("AP IP: %s (密码: %s)\n", ap_ip, AP_PASS);

    printf("Web界面可通过以上IP地址访问\n");
    
    printf("\n=== L298N电机控制信息 ===\n");
    printf("GPIO引脚连接：\n");
    printf("  IN1 -> GPIO 3  (左电机控制1)\n");
    printf("  IN2 -> GPIO 1  (左电机控制2)\n");  
    printf("  IN3 -> GPIO 2  (右电机控制1)\n");
    printf("  IN4 -> GPIO 42 (右电机控制2)\n");
    printf("电机控制已就绪，可通过Web API控制\n");
    printf("========================\n");
    
    printf("\n");
}