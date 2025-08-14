#ifndef NAVIGATION_SERVICE_H
#define NAVIGATION_SERVICE_H

#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "local_ai_service.h"

// 导航状态枚举
typedef enum {
    NAV_IDLE = 0,           // 空闲状态
    NAV_SCANNING,           // 原地360度扫描
    NAV_MOVING_FORWARD,     // 向前移动
    NAV_NAVIGATING_DOOR,    // 穿越门口
    NAV_EXPLORING_AREA,     // 探索新区域
    NAV_RETURNING_HOME,     // 返回起始点
    NAV_STUCK              // 卡住无法移动
} navigation_state_t;

// 搜索策略枚举
typedef enum {
    SEARCH_LOCAL_SCAN = 0,  // 本地360度扫描
    SEARCH_AREA_EXPAND,     // 区域扩展搜索
    SEARCH_DOOR_NAVIGATION, // 门口导航搜索
    SEARCH_COMPLETE         // 搜索完成
} search_strategy_t;

// 兴趣点结构体
typedef struct {
    float x, y;             // 相对位置坐标
    int point_type;         // 兴趣点类型 (门口=1, 路口=2)
    char description[32];   // 描述信息
    bool visited;           // 是否已访问
    time_t discovered_time; // 发现时间
} interest_point_t;

// 导航任务结构体
typedef struct {
    navigation_state_t nav_state;
    search_strategy_t current_strategy;
    char target_object[64];         // 目标物体
    int scan_angle;                 // 当前扫描角度
    int total_scan_cycles;          // 总扫描周期数
    int current_scan_cycle;         // 当前扫描周期
    float move_distance;            // 移动距离
    int move_steps;                 // 移动步数
    interest_point_t interest_points[10]; // 兴趣点数组
    int interest_point_count;       // 兴趣点数量
    bool is_stuck;                  // 是否卡住
    time_t last_move_time;          // 上次移动时间
    SemaphoreHandle_t nav_mutex;    // 导航互斥锁
} navigation_task_t;

/**
 * @brief 初始化导航服务
 * 
 * @return esp_err_t
 */
esp_err_t navigation_service_init(void);

/**
 * @brief 开始主动搜索任务
 * 
 * @param target_object 目标物体名称
 * @return esp_err_t
 */
esp_err_t navigation_start_search(const char* target_object);

/**
 * @brief 停止搜索任务
 * 
 * @return esp_err_t
 */
esp_err_t navigation_stop_search(void);

/**
 * @brief 获取当前导航状态
 * 
 * @return navigation_state_t
 */
navigation_state_t navigation_get_state(void);

/**
 * @brief 获取搜索进度
 * 
 * @return int 进度百分比 (0-100)
 */
int navigation_get_progress(void);

/**
 * @brief 执行360度原地扫描
 * 
 * @param target_object 目标物体
 * @return esp_err_t
 */
esp_err_t navigation_perform_360_scan(const char* target_object);

/**
 * @brief 执行区域扩展搜索
 * 
 * @return esp_err_t
 */
esp_err_t navigation_expand_search_area(void);

/**
 * @brief 识别并导航到门口/路口
 * 
 * @return esp_err_t
 */
esp_err_t navigation_navigate_to_door(void);

/**
 * @brief 检测机器人是否卡住
 * 
 * @return bool true=卡住, false=正常
 */
bool navigation_is_stuck(void);

/**
 * @brief 添加兴趣点
 * 
 * @param x 相对X坐标
 * @param y 相对Y坐标
 * @param type 兴趣点类型
 * @param description 描述
 * @return esp_err_t
 */
esp_err_t navigation_add_interest_point(float x, float y, int type, const char* description);

/**
 * @brief 获取导航任务信息
 * 
 * @return const navigation_task_t*
 */
const navigation_task_t* navigation_get_task_info(void);

/**
 * @brief 导航服务主循环任务
 * 
 * @param pvParameters 任务参数
 */
void navigation_service_task(void* pvParameters);

#endif // NAVIGATION_SERVICE_H