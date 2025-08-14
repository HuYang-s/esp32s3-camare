#ifndef LOCAL_AI_SERVICE_H
#define LOCAL_AI_SERVICE_H

#include "esp_err.h"
#include "camera_driver.h"

// AI任务状态枚举
typedef enum {
    AI_TASK_IDLE = 0,           // 空闲状态
    AI_TASK_SEARCHING,          // 正在搜索
    AI_TASK_COMPLETED,          // 任务完成
    AI_TASK_FAILED_TIMEOUT,     // 任务超时
    AI_TASK_FAILED_UNABLE,      // 无法完成
    AI_TASK_SCANNING,           // 正在360度扫描
    AI_TASK_MOVING,             // 正在移动搜索
    AI_TASK_NAVIGATING          // 正在导航到新区域
} ai_task_status_t;

// 检测结果结构体
typedef struct {
    float x, y, width, height;  // 边界框坐标 (归一化 0-1)
    float confidence;           // 置信度
    int class_id;              // 类别ID
    char class_name[32];       // 类别名称
} detection_result_t;

// AI任务结构体
typedef struct {
    ai_task_status_t status;
    char target_object[64];     // 目标物品名称
    int timeout_seconds;        // 任务超时时间
    time_t start_time;         // 任务开始时间
    detection_result_t results[10]; // 最多10个检测结果
    int result_count;          // 实际检测结果数量
    char status_message[128];  // 状态消息
    char detailed_log[512];    // 详细日志信息
    float progress;            // 任务进度 (0.0-1.0)
    bool use_navigation;       // 是否使用主动导航搜索
    int search_cycles;         // 搜索周期数
    int areas_explored;        // 已探索区域数
    bool target_found;         // 是否找到目标
    float target_confidence;   // 目标置信度
} ai_task_t;

/**
 * @brief 初始化本地AI服务
 * 
 * @return esp_err_t 
 */
esp_err_t local_ai_service_init(void);

/**
 * @brief 使用本地AI模型进行物体检测
 * 
 * @param fb 图像帧缓冲区
 * @param results 检测结果数组
 * @param max_results 最大结果数量
 * @return int 实际检测到的物体数量，-1表示失败
 */
int local_ai_detect_objects(camera_fb_t *fb, detection_result_t *results, int max_results);

/**
 * @brief 开始AI任务
 * 
 * @param target_object 目标物品名称
 * @param timeout_seconds 超时时间（秒）
 * @return esp_err_t 
 */
esp_err_t local_ai_start_task(const char *target_object, int timeout_seconds);

/**
 * @brief 停止当前AI任务
 * 
 * @return esp_err_t 
 */
esp_err_t local_ai_stop_task(void);

/**
 * @brief 获取当前AI任务状态
 * 
 * @return const ai_task_t* 
 */
const ai_task_t* local_ai_get_task_status(void);

/**
 * @brief 处理AI任务（在主循环中调用）
 * 
 * @param fb 当前图像帧
 * @return esp_err_t 
 */
esp_err_t local_ai_process_task(camera_fb_t *fb);

/**
 * @brief 在图像上绘制检测结果（用于调试可视化）
 * 
 * @param fb 图像帧缓冲区
 * @param results 检测结果数组
 * @param result_count 结果数量
 * @return esp_err_t 
 */
esp_err_t local_ai_draw_detections(camera_fb_t *fb, const detection_result_t *results, int result_count);

/**
 * @brief 获取类别名称
 * 
 * @param class_id 类别ID
 * @return const char* 类别名称
 */
const char* local_ai_get_class_name(int class_id);

/**
 * @brief 启动主动搜索任务（带导航）
 * 
 * @param target_object 目标物品名称
 * @param timeout_seconds 超时时间（秒）
 * @param use_navigation 是否使用导航搜索
 * @return esp_err_t 
 */
esp_err_t local_ai_start_navigation_task(const char *target_object, int timeout_seconds, bool use_navigation);

/**
 * @brief 获取任务进度百分比
 * 
 * @return int 进度百分比 (0-100)
 */
int local_ai_get_task_progress(void);

/**
 * @brief 获取详细的任务日志
 * 
 * @return const char* 日志字符串
 */
const char* local_ai_get_task_log(void);

/**
 * @brief 检查任务是否超时
 * 
 * @return bool true=超时, false=未超时
 */
bool local_ai_is_task_timeout(void);

/**
 * @brief 更新任务状态和进度
 * 
 * @param status 新状态
 * @param progress 进度 (0.0-1.0)
 * @param message 状态消息
 * @return esp_err_t
 */
esp_err_t local_ai_update_task_status(ai_task_status_t status, float progress, const char* message);

#endif // LOCAL_AI_SERVICE_H