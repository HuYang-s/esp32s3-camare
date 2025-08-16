/**
 * @file esp32s3_camera_web.c
 * @brief ESP32-S3摄像头AI分析系统主程序
 * 
 * 这是一个基于ESP32-S3的智能摄像头系统，集成了以下功能：
 * - 摄像头图像采集
 * - 本地AI物体检测
 * - 云端AI分析
 * - 电机控制（L298N驱动）
 * - WiFi网络管理（AP+STA双模式）
 * - Web服务器界面
 * - 图像存储管理
 * - 导航服务
 * 
 * @author ESP32-S3 Camera AI Project
 * @version 1.0
 */

// 标准C库头文件
#include <stdio.h>          // 标准输入输出函数
#include <string.h>         // 字符串处理函数

// FreeRTOS实时操作系统头文件
#include "freertos/FreeRTOS.h"  // FreeRTOS核心功能
#include "freertos/task.h"      // 任务管理
#include "freertos/semphr.h"    // 信号量和互斥锁

// ESP-IDF系统头文件
#include "esp_system.h"     // ESP32系统函数
#include "esp_log.h"        // 日志输出系统
#include "nvs_flash.h"      // 非易失性存储管理

// 项目自定义组件头文件
#include "camera_driver.h"      // 摄像头驱动接口
#include "wifi_manager.h"       // WiFi网络管理
#include "time_service.h"       // 时间服务
#include "storage_manager.h"    // 存储管理
#include "ai_service.h"         // AI分析服务
#include "web_server.h"         // Web服务器
#include "motor_driver.h"       // 电机驱动
#include "local_ai_service.h"   // 本地AI服务
#include "navigation_service.h" // 导航服务

// 日志标签，用于ESP_LOG系列函数
static const char *TAG = "main";

// WiFi AP模式的密码定义，仅用于启动时向用户显示
#define AP_PASS "12345678"

// 摄像头访问互斥锁，确保多任务环境下摄像头的安全访问
static SemaphoreHandle_t camera_mutex;

// AI分析模式控制变量
// false = 普通AI分析模式（本地AI + 云端AI回退）
// true = 自动驾驶模式（AI控制电机移动）
static bool ai_auto_drive_mode = false;

// 分析计数器，用于跟踪已进行的分析次数
static int analysis_counter = 0;

/**
 * @brief 电机测试任务函数
 * 
 * 这是一个FreeRTOS任务，用于测试L298N电机驱动的功能。
 * 任务启动后等待5秒让系统稳定，然后执行电机测试序列。
 * 测试完成后自动删除任务。
 * 
 * @param pvParameters 任务参数（未使用）
 */
static void motor_test_task(void *pvParameters)
{
    ESP_LOGI(TAG, "开始电机测试任务");
    
    // 等待5秒让系统稳定，避免启动时的干扰
    vTaskDelay(pdMS_TO_TICKS(5000));
    
    // 执行预定义的电机测试序列（前进、后退、左转、右转等）
    motor_test_sequence();
    
    ESP_LOGI(TAG, "电机测试完成，删除任务");
    // 测试完成后删除当前任务，释放内存
    vTaskDelete(NULL);
}

/**
 * @brief 图像采集和AI分析主任务
 * 
 * 这是系统的核心任务，负责：
 * 1. 定期从摄像头采集图像
 * 2. 保存图像到存储系统
 * 3. 调用AI服务进行图像分析
 * 4. 处理AI搜索任务
 * 5. 根据模式选择不同的AI分析策略
 * 
 * 任务运行在无限循环中，持续进行图像采集和分析。
 * 
 * @param pvParameters 任务参数（未使用）
 */
static void capture_and_analyze_task(void *pvParameters)
{
    camera_fb_t *fb = NULL;  // 摄像头帧缓冲区指针
    
    // 主循环：持续进行图像采集和分析
    while (1) {
        analysis_counter++;  // 增加分析计数器
        
        // 演示功能：每10次分析自动切换一次AI模式
        // 这样可以展示两种不同的AI分析模式
        if (analysis_counter % 10 == 5) {
            ai_auto_drive_mode = !ai_auto_drive_mode;  // 切换模式
            printf("\n=== 切换AI模式 ===\n");
            printf("当前模式: %s\n", ai_auto_drive_mode ? "AI自动驾驶" : "AI普通分析");
            printf("==================\n\n");
        }
        
        // 打印当前拍照信息和模式
        printf("\n=== 开始拍照 (模式: %s) ===\n", ai_auto_drive_mode ? "AI自动驾驶" : "AI普通分析");
        ESP_LOGI(TAG, "正在拍照...");
        
        // 尝试获取摄像头访问权限（互斥锁）
        // 等待最多500ms，确保在多任务环境下的安全访问
        if (xSemaphoreTake(camera_mutex, pdMS_TO_TICKS(500)) == pdTRUE) {
            // 从摄像头采集一帧图像
            esp_err_t res = camera_capture(&fb);
            
            // 检查图像采集是否成功
            if (res == ESP_OK && fb != NULL) {
                printf("拍照完成，图片大小: %u 字节\n", (unsigned int)fb->len);
                
                // 将图像保存到存储系统，获取保存的文件名
                char saved_filename[64];
                storage_manager_save_image(fb, saved_filename, sizeof(saved_filename));
                
                // 释放摄像头访问权限
                xSemaphoreGive(camera_mutex);
                
                // === AI分析处理流程 ===
                
                // 1. 处理AI搜索任务（如果有活动的搜索任务）
                // 这个函数会检查是否有用户通过Web界面启动的搜索任务
                esp_err_t search_result = ai_service_process_search_task(fb);
                if (search_result != ESP_OK) {
                    ESP_LOGD(TAG, "AI搜索任务处理出错");
                }
                
                // 2. 处理本地AI任务状态更新
                // 即使没有搜索任务，也要更新本地AI的状态
                esp_err_t local_ai_result = local_ai_process_task(fb);
                if (local_ai_result == ESP_OK) {
                    // 获取本地AI任务状态并显示
                    const ai_task_t* task_status = local_ai_get_task_status();
                    if (task_status->status != AI_TASK_IDLE) {
                        printf("本地AI任务状态: %s\n", task_status->status_message);
                    }
                }
                
                // 3. 根据当前模式选择不同的AI分析策略
                esp_err_t ai_result;
                if (ai_auto_drive_mode) {
                    // === 自动驾驶模式 ===
                    // 使用AI分析图像并自动控制电机移动
                    printf("开始AI自动驾驶分析...\n");
                    ai_result = ai_service_auto_drive_analyze(fb, saved_filename);
                    if (ai_result == ESP_OK) {
                        printf("AI自动驾驶分析完成\n");
                    } else {
                        printf("AI自动驾驶分析失败\n");
                    }
                } else {
                    // === 普通分析模式 ===
                    // 优先使用本地AI检测，如果没有结果则回退到云端AI
                    
                    // 尝试本地AI物体检测
                    detection_result_t local_results[5];  // 最多检测5个物体
                    int local_detection_count = local_ai_detect_objects(fb, local_results, 5);
                    
                    if (local_detection_count > 0) {
                        // 本地AI检测成功，使用本地结果
                        printf("本地AI检测到 %d 个物体:\n", local_detection_count);
                        char local_result_text[512] = "本地AI检测结果:\n";
                        
                        // 遍历所有检测结果
                        for (int i = 0; i < local_detection_count; i++) {
                            printf("- %s (置信度: %.2f)\n", 
                                   local_results[i].class_name, 
                                   local_results[i].confidence);
                            
                            // 构建结果文本用于存储
                            char item_text[64];
                            snprintf(item_text, sizeof(item_text), "- %s (置信度: %.2f)\n", 
                                local_results[i].class_name, local_results[i].confidence);
                            strcat(local_result_text, item_text);
                        }
                        
                        // 更新存储系统中的AI分析结果
                        storage_manager_update_ai_result(saved_filename, local_result_text);
                        ai_result = ESP_OK;
                    } else {
                        // 本地AI没有检测到物体，使用云端AI作为回退
                        ai_result = ai_service_analyze_image(fb, saved_filename);
                        if (ai_result == ESP_OK) {
                            printf("云端AI分析完成\n");
                        } else {
                            // 获取连续失败次数，用于错误恢复策略
                            int failure_count = ai_service_get_socket_failure_count();
                            printf("云端AI分析失败 (失败次数: %d)\n", failure_count);
                            
                            // 如果连续失败3次，等待5秒再重试
                            if (failure_count >= 3) {
                                printf("连续AI分析失败，等待5秒后重试...\n");
                                vTaskDelay(pdMS_TO_TICKS(5000));
                            }
                        }
                    }
                }
            
                // 释放摄像头帧缓冲区内存
                camera_return_fb(fb);
                printf("AI分析完成，立即进行下次拍照...\n\n");
            } else {
                // 图像采集失败的处理
                ESP_LOGE(TAG, "Camera capture failed");
                xSemaphoreGive(camera_mutex);  // 确保释放互斥锁
                printf("拍照失败，重试中...\n");
                vTaskDelay(pdMS_TO_TICKS(1000));  // 等待1秒后重试
            }
        } else {
            // 无法获取摄像头访问权限的处理
            ESP_LOGW(TAG, "无法获取摄像头访问权限");
            printf("无法获取摄像头访问权限，重试中...\n");
            vTaskDelay(pdMS_TO_TICKS(500));  // 等待500ms后重试
        }
        
        // === 错误恢复和延时控制 ===
        
        // 如果AI分析连续失败过多，增加延时避免过度重试
        if (ai_service_get_socket_failure_count() >= 5) {
            printf("连续AI分析失败过多，暂停3秒...\n");
            vTaskDelay(pdMS_TO_TICKS(3000));
        } else {
            // 正常情况下的延时控制
            // 自动驾驶模式需要更长的处理时间，避免过于频繁的控制
            int delay_ms = ai_auto_drive_mode ? 3000 : 1000;
            vTaskDelay(pdMS_TO_TICKS(delay_ms));
        }
    }
}

/**
 * @brief 应用程序主入口函数
 * 
 * 这是ESP32应用程序的主入口点，负责：
 * 1. 初始化所有系统组件
 * 2. 启动各种服务
 * 3. 创建主要的FreeRTOS任务
 * 4. 显示系统启动信息
 * 
 * 函数执行完毕后，系统将由FreeRTOS调度器接管，
 * 各个任务开始独立运行。
 */
void app_main(void)
{
    // 显示系统启动信息和可用内存
    ESP_LOGI(TAG, "Starting application - Free heap: %u bytes", 
             (unsigned int)esp_get_free_heap_size());
    
    // === 初始化NVS（非易失性存储）===
    // NVS用于存储WiFi配置、系统设置等持久化数据
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        // 如果NVS分区损坏或版本不匹配，擦除并重新初始化
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);  // 确保NVS初始化成功

    // === 创建摄像头访问互斥锁 ===
    // 确保在多任务环境下摄像头的安全访问
    camera_mutex = xSemaphoreCreateMutex();
    if (camera_mutex == NULL) {
        ESP_LOGE(TAG, "Failed to create camera mutex");
        return;  // 互斥锁创建失败，无法继续运行
    }
    
    // === 初始化存储管理系统 ===
    // 用于管理图像文件的保存和读取
    ESP_ERROR_CHECK(storage_manager_init());

    // === 初始化WiFi网络 ===
    ESP_LOGI(TAG, "初始化WiFi AP+STA模式...");
    wifi_manager_init_ap_sta();  // 同时启动AP和STA模式
    
    // 等待WiFi连接，最多等待30秒
    if (wifi_manager_wait_for_connect(pdMS_TO_TICKS(30000)) == ESP_OK) {
        ESP_LOGI(TAG, "WiFi连接成功");
        printf("WiFi连接成功，开始初始化服务...\n");
        // WiFi连接成功后初始化时间服务（需要网络同步时间）
        time_service_init();
    } else {
        ESP_LOGE(TAG, "WiFi连接超时");
        printf("WiFi连接失败，请检查网络配置\n");
        // 即使WiFi连接失败，也继续执行，因为AP模式可能仍然可用
    }

    // === 初始化摄像头 ===
    ESP_ERROR_CHECK(camera_init());
    printf("摄像头初始化完成\n");

    // === 初始化AI分析服务 ===
    ESP_LOGI(TAG, "初始化AI服务...");
    esp_err_t ai_ret = ai_service_init();
    if (ai_ret == ESP_OK) {
        printf("AI服务初始化成功\n");
    } else {
        ESP_LOGE(TAG, "AI服务初始化失败");
        printf("AI服务初始化失败\n");
        // 注意：AI服务初始化失败不会终止程序，系统仍可运行
    }

    // === 初始化导航服务 ===
    ESP_LOGI(TAG, "初始化导航服务...");
    esp_err_t nav_ret = navigation_service_init();
    if (nav_ret == ESP_OK) {
        printf("导航服务初始化成功\n");
    } else {
        ESP_LOGE(TAG, "导航服务初始化失败");
        printf("导航服务初始化失败\n");
    }

    // === 初始化L298N电机驱动 ===
    ESP_LOGI(TAG, "初始化L298N电机驱动...");
    esp_err_t motor_ret = motor_driver_init();
    if (motor_ret == ESP_OK) {
        printf("L298N电机驱动初始化成功\n");
        // 显示GPIO引脚映射信息，方便用户连接硬件
        printf("GPIO引脚映射: IN1=GPIO%d, IN2=GPIO%d, IN3=GPIO%d, IN4=GPIO%d\n", 
               3, 1, 2, 42);
    } else {
        ESP_LOGE(TAG, "L298N电机驱动初始化失败");
        printf("L298N电机驱动初始化失败\n");
    }

    // === 启动Web服务器 ===
    // Web服务器提供用户界面和API接口
    web_server_start();

    // === 创建主要的FreeRTOS任务 ===
    
    // 创建图像采集和AI分析任务（核心任务）
    // 堆栈大小：12288字节，优先级：5（较高优先级）
    xTaskCreate(capture_and_analyze_task, "capture_analyze_task", 12288, NULL, 5, NULL);

    // 如果电机驱动初始化成功，创建电机测试任务
    if (motor_ret == ESP_OK) {
        // 堆栈大小：4096字节，优先级：3（中等优先级）
        xTaskCreate(motor_test_task, "motor_test_task", 4096, NULL, 3, NULL);
        printf("电机测试任务已启动，将在5秒后开始测试\n");
    }

    // === 显示系统启动完成信息 ===
    printf("\n系统启动完成，开始自动拍照和AI分析\n");
    printf("网络模式：AP+STA双模式\n");
    
    // 获取并显示STA模式的IP地址
    char sta_ip[16];
    wifi_manager_get_sta_ip(sta_ip, sizeof(sta_ip));
    printf("STA IP: %s\n", sta_ip);

    // 获取并显示AP模式的IP地址和密码
    char ap_ip[16];
    wifi_manager_get_ap_ip(ap_ip, sizeof(ap_ip));
    printf("AP IP: %s (密码: %s)\n", ap_ip, AP_PASS);

    printf("Web界面可通过以上IP地址访问\n");
    
    // === 显示硬件连接信息 ===
    printf("\n=== L298N电机控制信息 ===\n");
    printf("GPIO引脚连接：\n");
    printf("  IN1 -> GPIO 3  (左电机控制1)\n");
    printf("  IN2 -> GPIO 1  (左电机控制2)\n");  
    printf("  IN3 -> GPIO 2  (右电机控制1)\n");
    printf("  IN4 -> GPIO 42 (右电机控制2)\n");
    printf("电机控制已就绪，可通过Web API控制\n");
    printf("========================\n");
    
    printf("\n");
    
    // 函数执行完毕，FreeRTOS调度器接管系统
    // 各个任务开始独立运行，主函数不会返回
}