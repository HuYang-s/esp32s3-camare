#include <stdio.h>
#include <string.h>
#include <time.h>
#include <math.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "esp_log.h"
#include "esp_timer.h"

#include "navigation_service.h"
#include "motor_driver.h"
#include "camera_driver.h"
#include "local_ai_service.h"

static const char *TAG = "navigation";

// 全局导航任务状态
static navigation_task_t nav_task = {0};
static TaskHandle_t nav_task_handle = NULL;
static bool nav_service_running = false;

// 导航参数配置
#define SCAN_ANGLE_STEP 15          // 每次旋转15度
#define SCAN_TOTAL_ANGLES 24        // 360/15 = 24个角度
#define MOVE_FORWARD_DISTANCE 50    // 向前移动50cm
#define MOVE_STEP_TIME_MS 100       // 每步移动时间
#define STUCK_DETECTION_TIME_S 5    // 卡住检测时间
#define MAX_SEARCH_CYCLES 5         // 最大搜索周期

// 门口和路口检测关键词
static const char* door_keywords[] = {"door", "doorway", "entrance", "exit", "gate"};
static const char* passage_keywords[] = {"hallway", "corridor", "passage", "path", "way"};

// 前向声明
static bool is_door_or_passage(const detection_result_t* result);

esp_err_t navigation_service_init(void)
{
    ESP_LOGI(TAG, "初始化导航服务...");
    
    // 初始化导航任务结构
    memset(&nav_task, 0, sizeof(navigation_task_t));
    nav_task.nav_state = NAV_IDLE;
    nav_task.current_strategy = SEARCH_LOCAL_SCAN;
    nav_task.total_scan_cycles = MAX_SEARCH_CYCLES;
    
    // 创建互斥锁
    nav_task.nav_mutex = xSemaphoreCreateMutex();
    if (nav_task.nav_mutex == NULL) {
        ESP_LOGE(TAG, "创建导航互斥锁失败");
        return ESP_FAIL;
    }
    
    // 创建导航服务任务
    BaseType_t ret = xTaskCreate(navigation_service_task, "nav_service", 4096, NULL, 5, &nav_task_handle);
    if (ret != pdPASS) {
        ESP_LOGE(TAG, "创建导航服务任务失败");
        vSemaphoreDelete(nav_task.nav_mutex);
        return ESP_FAIL;
    }
    
    nav_service_running = true;
    ESP_LOGI(TAG, "导航服务初始化完成");
    return ESP_OK;
}

esp_err_t navigation_start_search(const char* target_object)
{
    if (!nav_service_running) {
        ESP_LOGE(TAG, "导航服务未初始化");
        return ESP_FAIL;
    }
    
    if (xSemaphoreTake(nav_task.nav_mutex, pdMS_TO_TICKS(1000)) != pdTRUE) {
        ESP_LOGE(TAG, "获取导航互斥锁失败");
        return ESP_FAIL;
    }
    
    // 设置搜索目标
    strncpy(nav_task.target_object, target_object, sizeof(nav_task.target_object) - 1);
    nav_task.target_object[sizeof(nav_task.target_object) - 1] = '\0';
    
    // 重置搜索状态
    nav_task.nav_state = NAV_SCANNING;
    nav_task.current_strategy = SEARCH_LOCAL_SCAN;
    nav_task.scan_angle = 0;
    nav_task.current_scan_cycle = 0;
    nav_task.move_distance = 0;
    nav_task.move_steps = 0;
    nav_task.interest_point_count = 0;
    nav_task.is_stuck = false;
    nav_task.last_move_time = time(NULL);
    
    xSemaphoreGive(nav_task.nav_mutex);
    
    ESP_LOGI(TAG, "开始搜索目标: %s", target_object);
    return ESP_OK;
}

esp_err_t navigation_stop_search(void)
{
    if (xSemaphoreTake(nav_task.nav_mutex, pdMS_TO_TICKS(1000)) != pdTRUE) {
        return ESP_FAIL;
    }
    
    // 停止所有电机
    motor_stop();
    
    // 重置状态
    nav_task.nav_state = NAV_IDLE;
    nav_task.current_strategy = SEARCH_LOCAL_SCAN;
    
    xSemaphoreGive(nav_task.nav_mutex);
    
    ESP_LOGI(TAG, "搜索任务已停止");
    return ESP_OK;
}

navigation_state_t navigation_get_state(void)
{
    navigation_state_t state = NAV_IDLE;
    if (xSemaphoreTake(nav_task.nav_mutex, pdMS_TO_TICKS(100)) == pdTRUE) {
        state = nav_task.nav_state;
        xSemaphoreGive(nav_task.nav_mutex);
    }
    return state;
}

int navigation_get_progress(void)
{
    int progress = 0;
    if (xSemaphoreTake(nav_task.nav_mutex, pdMS_TO_TICKS(100)) == pdTRUE) {
        switch (nav_task.current_strategy) {
            case SEARCH_LOCAL_SCAN:
                progress = (nav_task.scan_angle * 100) / 360;
                break;
            case SEARCH_AREA_EXPAND:
                progress = 25 + (nav_task.move_steps * 25) / 10;
                break;
            case SEARCH_DOOR_NAVIGATION:
                progress = 50 + (nav_task.current_scan_cycle * 25) / nav_task.total_scan_cycles;
                break;
            case SEARCH_COMPLETE:
                progress = 100;
                break;
            default:
                progress = 0;
                break;
        }
        
        // 限制在0-100范围内
        if (progress > 100) progress = 100;
        if (progress < 0) progress = 0;
        
        xSemaphoreGive(nav_task.nav_mutex);
    }
    return progress;
}

esp_err_t navigation_perform_360_scan(const char* target_object)
{
    ESP_LOGI(TAG, "开始360度扫描搜索: %s", target_object);
    
    for (int angle = 0; angle < 360; angle += SCAN_ANGLE_STEP) {
        // 更新扫描角度
        if (xSemaphoreTake(nav_task.nav_mutex, pdMS_TO_TICKS(100)) == pdTRUE) {
            nav_task.scan_angle = angle;
            xSemaphoreGive(nav_task.nav_mutex);
        }
        
        // 原地右转
        ESP_LOGI(TAG, "旋转到角度: %d度", angle);
        motor_turn_right();
        vTaskDelay(pdMS_TO_TICKS(500)); // 旋转0.5秒
        motor_stop();
        vTaskDelay(pdMS_TO_TICKS(200)); // 稳定0.2秒
        
        // 拍照并进行AI识别
        camera_fb_t *fb = camera_capture();
        if (fb != NULL) {
            detection_result_t results[5];
            int result_count = 0;
            
            // 执行本地AI检测
            esp_err_t ai_result = local_ai_detect_objects(fb, results, 5, &result_count);
            
            if (ai_result == ESP_OK && result_count > 0) {
                // 检查是否找到目标物体
                for (int i = 0; i < result_count; i++) {
                    ESP_LOGI(TAG, "检测到物体: %s (置信度: %.2f)", 
                             results[i].class_name, results[i].confidence);
                    
                    // 检查是否是目标物体
                    if (strstr(results[i].class_name, target_object) != NULL && 
                        results[i].confidence > 0.5) {
                        ESP_LOGI(TAG, "找到目标物体: %s", target_object);
                        camera_fb_return(fb);
                        return ESP_OK; // 找到目标
                    }
                    
                    // 检查是否是门口或路口
                    if (is_door_or_passage(&results[i])) {
                        navigation_add_interest_point(
                            results[i].x, results[i].y, 
                            (strstr(results[i].class_name, "door") ? 1 : 2),
                            results[i].class_name
                        );
                    }
                }
            }
            
            camera_fb_return(fb);
        }
        
        // 检查是否需要停止搜索
        if (navigation_get_state() == NAV_IDLE) {
            ESP_LOGI(TAG, "搜索任务被停止");
            return ESP_FAIL;
        }
        
        vTaskDelay(pdMS_TO_TICKS(100));
    }
    
    ESP_LOGI(TAG, "360度扫描完成，未找到目标物体");
    return ESP_FAIL; // 未找到目标
}

esp_err_t navigation_expand_search_area(void)
{
    ESP_LOGI(TAG, "开始扩大搜索区域");
    
    // 向前移动一段距离
    ESP_LOGI(TAG, "向前移动 %d cm", MOVE_FORWARD_DISTANCE);
    
    int move_steps = MOVE_FORWARD_DISTANCE / 5; // 每步5cm
    for (int step = 0; step < move_steps; step++) {
        // 更新移动步数
        if (xSemaphoreTake(nav_task.nav_mutex, pdMS_TO_TICKS(100)) == pdTRUE) {
            nav_task.move_steps = step;
            xSemaphoreGive(nav_task.nav_mutex);
        }
        
        // 向前移动一小步
        motor_move_forward();
        vTaskDelay(pdMS_TO_TICKS(MOVE_STEP_TIME_MS));
        motor_stop();
        
        // 检查是否卡住
        if (navigation_is_stuck()) {
            ESP_LOGW(TAG, "机器人可能被卡住，停止移动");
            if (xSemaphoreTake(nav_task.nav_mutex, pdMS_TO_TICKS(100)) == pdTRUE) {
                nav_task.nav_state = NAV_STUCK;
                nav_task.is_stuck = true;
                xSemaphoreGive(nav_task.nav_mutex);
            }
            return ESP_FAIL;
        }
        
        // 检查是否需要停止
        if (navigation_get_state() == NAV_IDLE) {
            return ESP_FAIL;
        }
        
        vTaskDelay(pdMS_TO_TICKS(50));
    }
    
    ESP_LOGI(TAG, "移动完成，准备进行新的360度扫描");
    return ESP_OK;
}

esp_err_t navigation_navigate_to_door(void)
{
    ESP_LOGI(TAG, "开始导航到门口/路口");
    
    // 查找最近的未访问兴趣点
    interest_point_t* target_point = NULL;
    float min_distance = 1000.0;
    
    if (xSemaphoreTake(nav_task.nav_mutex, pdMS_TO_TICKS(100)) == pdTRUE) {
        for (int i = 0; i < nav_task.interest_point_count; i++) {
            if (!nav_task.interest_points[i].visited) {
                float distance = sqrt(pow(nav_task.interest_points[i].x, 2) + 
                                    pow(nav_task.interest_points[i].y, 2));
                if (distance < min_distance) {
                    min_distance = distance;
                    target_point = &nav_task.interest_points[i];
                }
            }
        }
        xSemaphoreGive(nav_task.nav_mutex);
    }
    
    if (target_point == NULL) {
        ESP_LOGI(TAG, "没有找到可导航的兴趣点");
        return ESP_FAIL;
    }
    
    ESP_LOGI(TAG, "导航到兴趣点: %s (距离: %.1f)", target_point->description, min_distance);
    
    // 简单的导航策略：朝着目标点方向移动
    float angle = atan2(target_point->y, target_point->x) * 180.0 / M_PI;
    
    // 先旋转到目标方向
    if (angle > 0) {
        motor_turn_left();
    } else {
        motor_turn_right();
    }
    vTaskDelay(pdMS_TO_TICKS(abs((int)angle) * 10)); // 根据角度调整旋转时间
    motor_stop();
    
    // 向前移动
    int move_time = (int)(min_distance * 100); // 根据距离调整移动时间
    motor_move_forward();
    vTaskDelay(pdMS_TO_TICKS(move_time));
    motor_stop();
    
    // 标记该兴趣点为已访问
    if (xSemaphoreTake(nav_task.nav_mutex, pdMS_TO_TICKS(100)) == pdTRUE) {
        target_point->visited = true;
        xSemaphoreGive(nav_task.nav_mutex);
    }
    
    ESP_LOGI(TAG, "到达兴趣点，开始新区域搜索");
    return ESP_OK;
}

bool navigation_is_stuck(void)
{
    time_t current_time = time(NULL);
    bool stuck = false;
    
    if (xSemaphoreTake(nav_task.nav_mutex, pdMS_TO_TICKS(100)) == pdTRUE) {
        // 简单的卡住检测：如果长时间没有成功移动
        if (current_time - nav_task.last_move_time > STUCK_DETECTION_TIME_S) {
            stuck = true;
            nav_task.is_stuck = true;
        }
        xSemaphoreGive(nav_task.nav_mutex);
    }
    
    return stuck;
}

esp_err_t navigation_add_interest_point(float x, float y, int type, const char* description)
{
    if (xSemaphoreTake(nav_task.nav_mutex, pdMS_TO_TICKS(100)) != pdTRUE) {
        return ESP_FAIL;
    }
    
    if (nav_task.interest_point_count >= 10) {
        ESP_LOGW(TAG, "兴趣点数组已满");
        xSemaphoreGive(nav_task.nav_mutex);
        return ESP_FAIL;
    }
    
    interest_point_t* point = &nav_task.interest_points[nav_task.interest_point_count];
    point->x = x;
    point->y = y;
    point->point_type = type;
    strncpy(point->description, description, sizeof(point->description) - 1);
    point->description[sizeof(point->description) - 1] = '\0';
    point->visited = false;
    point->discovered_time = time(NULL);
    
    nav_task.interest_point_count++;
    
    xSemaphoreGive(nav_task.nav_mutex);
    
    ESP_LOGI(TAG, "添加兴趣点: %s (%.1f, %.1f)", description, x, y);
    return ESP_OK;
}

const navigation_task_t* navigation_get_task_info(void)
{
    return &nav_task;
}

// 辅助函数：检查是否是门口或路口
static bool is_door_or_passage(const detection_result_t* result)
{
    // 检查门口关键词
    for (int i = 0; i < sizeof(door_keywords) / sizeof(door_keywords[0]); i++) {
        if (strstr(result->class_name, door_keywords[i]) != NULL) {
            return true;
        }
    }
    
    // 检查路口关键词
    for (int i = 0; i < sizeof(passage_keywords) / sizeof(passage_keywords[0]); i++) {
        if (strstr(result->class_name, passage_keywords[i]) != NULL) {
            return true;
        }
    }
    
    return false;
}

void navigation_service_task(void* pvParameters)
{
    ESP_LOGI(TAG, "导航服务任务启动");
    
    while (nav_service_running) {
        navigation_state_t current_state = navigation_get_state();
        
        switch (current_state) {
            case NAV_IDLE:
                // 空闲状态，等待搜索命令
                vTaskDelay(pdMS_TO_TICKS(1000));
                break;
                
            case NAV_SCANNING:
                // 执行360度扫描
                if (navigation_perform_360_scan(nav_task.target_object) == ESP_OK) {
                    // 找到目标，任务完成
                    if (xSemaphoreTake(nav_task.nav_mutex, pdMS_TO_TICKS(100)) == pdTRUE) {
                        nav_task.nav_state = NAV_IDLE;
                        nav_task.current_strategy = SEARCH_COMPLETE;
                        xSemaphoreGive(nav_task.nav_mutex);
                    }
                    ESP_LOGI(TAG, "搜索任务完成");
                } else {
                    // 未找到目标，切换到区域扩展搜索
                    if (xSemaphoreTake(nav_task.nav_mutex, pdMS_TO_TICKS(100)) == pdTRUE) {
                        nav_task.nav_state = NAV_MOVING_FORWARD;
                        nav_task.current_strategy = SEARCH_AREA_EXPAND;
                        xSemaphoreGive(nav_task.nav_mutex);
                    }
                }
                break;
                
            case NAV_MOVING_FORWARD:
                // 扩大搜索区域
                if (navigation_expand_search_area() == ESP_OK) {
                    // 移动成功，继续扫描
                    if (xSemaphoreTake(nav_task.nav_mutex, pdMS_TO_TICKS(100)) == pdTRUE) {
                        nav_task.nav_state = NAV_SCANNING;
                        nav_task.current_scan_cycle++;
                        
                        // 检查是否超过最大搜索周期
                        if (nav_task.current_scan_cycle >= nav_task.total_scan_cycles) {
                            if (nav_task.interest_point_count > 0) {
                                // 有兴趣点，尝试导航
                                nav_task.nav_state = NAV_NAVIGATING_DOOR;
                                nav_task.current_strategy = SEARCH_DOOR_NAVIGATION;
                            } else {
                                // 没有兴趣点，搜索失败
                                nav_task.nav_state = NAV_IDLE;
                                nav_task.current_strategy = SEARCH_COMPLETE;
                                ESP_LOGI(TAG, "搜索失败：未找到目标物体");
                            }
                        }
                        
                        xSemaphoreGive(nav_task.nav_mutex);
                    }
                }
                break;
                
            case NAV_NAVIGATING_DOOR:
                // 导航到门口/路口
                if (navigation_navigate_to_door() == ESP_OK) {
                    // 导航成功，在新区域继续搜索
                    if (xSemaphoreTake(nav_task.nav_mutex, pdMS_TO_TICKS(100)) == pdTRUE) {
                        nav_task.nav_state = NAV_SCANNING;
                        nav_task.current_scan_cycle = 0; // 重置扫描周期
                        xSemaphoreGive(nav_task.nav_mutex);
                    }
                } else {
                    // 导航失败，结束搜索
                    if (xSemaphoreTake(nav_task.nav_mutex, pdMS_TO_TICKS(100)) == pdTRUE) {
                        nav_task.nav_state = NAV_IDLE;
                        nav_task.current_strategy = SEARCH_COMPLETE;
                        xSemaphoreGive(nav_task.nav_mutex);
                    }
                    ESP_LOGI(TAG, "导航失败，搜索结束");
                }
                break;
                
            case NAV_STUCK:
                // 机器人卡住，尝试脱困
                ESP_LOGW(TAG, "机器人卡住，尝试脱困");
                motor_move_backward();
                vTaskDelay(pdMS_TO_TICKS(1000));
                motor_turn_left();
                vTaskDelay(pdMS_TO_TICKS(500));
                motor_stop();
                
                // 重置卡住状态
                if (xSemaphoreTake(nav_task.nav_mutex, pdMS_TO_TICKS(100)) == pdTRUE) {
                    nav_task.nav_state = NAV_SCANNING;
                    nav_task.is_stuck = false;
                    nav_task.last_move_time = time(NULL);
                    xSemaphoreGive(nav_task.nav_mutex);
                }
                break;
                
            default:
                vTaskDelay(pdMS_TO_TICKS(100));
                break;
        }
        
        vTaskDelay(pdMS_TO_TICKS(100));
    }
    
    ESP_LOGI(TAG, "导航服务任务结束");
    vTaskDelete(NULL);
}