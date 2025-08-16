#ifndef MOTOR_DRIVER_H
#define MOTOR_DRIVER_H

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

// L298N GPIO引脚定义 - 避免与摄像头冲突
// 摄像头使用的引脚: 4,5,6,7,8,9,10,11,12,13,15,16,17,18
// 可用引脚: 1,2,3,19,20,21,35,36,37,38,39,40,41,42,45,46,47,48

#define MOTOR_IN1_GPIO  3   // 原来的GPIO 3
#define MOTOR_IN2_GPIO  1   // 改为GPIO 1 (避免与GPIO 8冲突)  
#define MOTOR_IN3_GPIO  2   // 改为GPIO 2 (避免与GPIO 18冲突)
#define MOTOR_IN4_GPIO  42  // 改为GPIO 42 (避免与GPIO 17冲突)

// 电机方向枚举
typedef enum {
    MOTOR_STOP = 0,
    MOTOR_FORWARD,
    MOTOR_BACKWARD,
    MOTOR_LEFT,
    MOTOR_RIGHT
} motor_direction_t;

// 电机速度等级 (0-100)
typedef enum {
    MOTOR_SPEED_STOP = 0,
    MOTOR_SPEED_LOW = 30,
    MOTOR_SPEED_MEDIUM = 60,
    MOTOR_SPEED_HIGH = 100
} motor_speed_t;

/**
 * @brief 初始化L298N电机驱动
 * @return ESP_OK 成功, ESP_FAIL 失败
 */
esp_err_t motor_driver_init(void);

/**
 * @brief 设置电机方向和速度
 * @param direction 电机方向
 * @param speed 电机速度 (0-100)
 * @return ESP_OK 成功, ESP_FAIL 失败
 */
esp_err_t motor_set_direction_speed(motor_direction_t direction, int speed);

/**
 * @brief 停止所有电机
 * @return ESP_OK 成功, ESP_FAIL 失败
 */
esp_err_t motor_stop_all(void);

/**
 * @brief 前进
 * @param speed 速度 (0-100)
 * @return ESP_OK 成功, ESP_FAIL 失败
 */
esp_err_t motor_forward(int speed);

/**
 * @brief 后退
 * @param speed 速度 (0-100)
 * @return ESP_OK 成功, ESP_FAIL 失败
 */
esp_err_t motor_backward(int speed);

/**
 * @brief 左转
 * @param speed 速度 (0-100)
 * @return ESP_OK 成功, ESP_FAIL 失败
 */
esp_err_t motor_left(int speed);

/**
 * @brief 右转
 * @param speed 速度 (0-100)
 * @return ESP_OK 成功, ESP_FAIL 失败
 */
esp_err_t motor_right(int speed);

/**
 * @brief 测试电机功能
 * @return ESP_OK 成功, ESP_FAIL 失败
 */
esp_err_t motor_test_sequence(void);

/**
 * @brief 按指定角度转弯
 * @param angle 转弯角度 (-180 到 180 度，负数为左转，正数为右转)
 * @param speed 转弯速度 (0-100)
 * @return ESP_OK 成功, ESP_FAIL 失败
 */
esp_err_t motor_turn_angle(int angle, int speed);

/**
 * @brief 差速转弯 (左右轮不同速度)
 * @param left_speed 左轮速度 (-100 到 100，负数为反转)
 * @param right_speed 右轮速度 (-100 到 100，负数为反转)
 * @return ESP_OK 成功, ESP_FAIL 失败
 */
esp_err_t motor_differential_drive(int left_speed, int right_speed);

/**
 * @brief 原地转弯
 * @param angle 转弯角度 (-180 到 180 度，负数为左转，正数为右转)
 * @param speed 转弯速度 (0-100)
 * @return ESP_OK 成功, ESP_FAIL 失败
 */
esp_err_t motor_pivot_turn(int angle, int speed);

#ifdef __cplusplus
}
#endif

#endif // MOTOR_DRIVER_H