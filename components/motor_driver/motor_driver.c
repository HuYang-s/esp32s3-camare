#include "motor_driver.h"
#include "esp_log.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char* TAG = "motor_driver";

// 初始化状态标志
static bool motor_initialized = false;

esp_err_t motor_driver_init(void)
{
    ESP_LOGI(TAG, "初始化L298N电机驱动...");
    
    // 配置GPIO为输出模式
    gpio_config_t io_conf = {0};
    io_conf.intr_type = GPIO_INTR_DISABLE;
    io_conf.mode = GPIO_MODE_OUTPUT;
    io_conf.pin_bit_mask = ((1ULL << MOTOR_IN1_GPIO) | 
                           (1ULL << MOTOR_IN2_GPIO) | 
                           (1ULL << MOTOR_IN3_GPIO) | 
                           (1ULL << MOTOR_IN4_GPIO));
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
    
    esp_err_t ret = gpio_config(&io_conf);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "GPIO配置失败: %s", esp_err_to_name(ret));
        return ret;
    }
    
    // 初始化所有引脚为低电平（停止状态）
    gpio_set_level(MOTOR_IN1_GPIO, 0);
    gpio_set_level(MOTOR_IN2_GPIO, 0);
    gpio_set_level(MOTOR_IN3_GPIO, 0);
    gpio_set_level(MOTOR_IN4_GPIO, 0);
    
    motor_initialized = true;
    ESP_LOGI(TAG, "L298N电机驱动初始化成功");
    ESP_LOGI(TAG, "GPIO引脚映射: IN1=%d, IN2=%d, IN3=%d, IN4=%d", 
             MOTOR_IN1_GPIO, MOTOR_IN2_GPIO, MOTOR_IN3_GPIO, MOTOR_IN4_GPIO);
    
    return ESP_OK;
}

esp_err_t motor_set_direction_speed(motor_direction_t direction, int speed)
{
    if (!motor_initialized) {
        ESP_LOGE(TAG, "电机驱动未初始化");
        return ESP_FAIL;
    }
    
    if (speed < 0) speed = 0;
    if (speed > 100) speed = 100;
    
    ESP_LOGI(TAG, "设置电机方向: %d, 速度: %d%%", direction, speed);
    
    switch (direction) {
        case MOTOR_STOP:
            return motor_stop_all();
            
        case MOTOR_FORWARD:
            return motor_forward(speed);
            
        case MOTOR_BACKWARD:
            return motor_backward(speed);
            
        case MOTOR_LEFT:
            return motor_left(speed);
            
        case MOTOR_RIGHT:
            return motor_right(speed);
            
        default:
            ESP_LOGE(TAG, "未知的电机方向: %d", direction);
            return ESP_FAIL;
    }
}

esp_err_t motor_stop_all(void)
{
    if (!motor_initialized) {
        return ESP_FAIL;
    }
    
    gpio_set_level(MOTOR_IN1_GPIO, 0);
    gpio_set_level(MOTOR_IN2_GPIO, 0);
    gpio_set_level(MOTOR_IN3_GPIO, 0);
    gpio_set_level(MOTOR_IN4_GPIO, 0);
    
    ESP_LOGI(TAG, "所有电机停止");
    return ESP_OK;
}

esp_err_t motor_forward(int speed)
{
    if (!motor_initialized) {
        return ESP_FAIL;
    }
    
    if (speed <= 0) {
        return motor_stop_all();
    }
    
    // 左电机前进: IN1=1, IN2=0
    // 右电机前进: IN3=1, IN4=0
    gpio_set_level(MOTOR_IN1_GPIO, 1);
    gpio_set_level(MOTOR_IN2_GPIO, 0);
    gpio_set_level(MOTOR_IN3_GPIO, 1);
    gpio_set_level(MOTOR_IN4_GPIO, 0);
    
    ESP_LOGI(TAG, "电机前进，速度: %d%%", speed);
    return ESP_OK;
}

esp_err_t motor_backward(int speed)
{
    if (!motor_initialized) {
        return ESP_FAIL;
    }
    
    if (speed <= 0) {
        return motor_stop_all();
    }
    
    // 左电机后退: IN1=0, IN2=1
    // 右电机后退: IN3=0, IN4=1
    gpio_set_level(MOTOR_IN1_GPIO, 0);
    gpio_set_level(MOTOR_IN2_GPIO, 1);
    gpio_set_level(MOTOR_IN3_GPIO, 0);
    gpio_set_level(MOTOR_IN4_GPIO, 1);
    
    ESP_LOGI(TAG, "电机后退，速度: %d%%", speed);
    return ESP_OK;
}

esp_err_t motor_left(int speed)
{
    if (!motor_initialized) {
        return ESP_FAIL;
    }
    
    if (speed <= 0) {
        return motor_stop_all();
    }
    
    // 左转：左电机后退，右电机前进
    // 左电机: IN1=0, IN2=1
    // 右电机: IN3=1, IN4=0
    gpio_set_level(MOTOR_IN1_GPIO, 0);
    gpio_set_level(MOTOR_IN2_GPIO, 1);
    gpio_set_level(MOTOR_IN3_GPIO, 1);
    gpio_set_level(MOTOR_IN4_GPIO, 0);
    
    ESP_LOGI(TAG, "电机左转，速度: %d%%", speed);
    return ESP_OK;
}

esp_err_t motor_right(int speed)
{
    if (!motor_initialized) {
        return ESP_FAIL;
    }
    
    if (speed <= 0) {
        return motor_stop_all();
    }
    
    // 右转：左电机前进，右电机后退
    // 左电机: IN1=1, IN2=0
    // 右电机: IN3=0, IN4=1
    gpio_set_level(MOTOR_IN1_GPIO, 1);
    gpio_set_level(MOTOR_IN2_GPIO, 0);
    gpio_set_level(MOTOR_IN3_GPIO, 0);
    gpio_set_level(MOTOR_IN4_GPIO, 1);
    
    ESP_LOGI(TAG, "电机右转，速度: %d%%", speed);
    return ESP_OK;
}

esp_err_t motor_test_sequence(void)
{
    if (!motor_initialized) {
        ESP_LOGE(TAG, "电机驱动未初始化，无法执行测试");
        return ESP_FAIL;
    }
    
    ESP_LOGI(TAG, "开始电机测试序列...");
    
    // 测试前进
    ESP_LOGI(TAG, "测试前进...");
    motor_forward(MOTOR_SPEED_MEDIUM);
    vTaskDelay(pdMS_TO_TICKS(2000));
    
    // 测试停止
    ESP_LOGI(TAG, "测试停止...");
    motor_stop_all();
    vTaskDelay(pdMS_TO_TICKS(1000));
    
    // 测试后退
    ESP_LOGI(TAG, "测试后退...");
    motor_backward(MOTOR_SPEED_MEDIUM);
    vTaskDelay(pdMS_TO_TICKS(2000));
    
    // 测试停止
    ESP_LOGI(TAG, "测试停止...");
    motor_stop_all();
    vTaskDelay(pdMS_TO_TICKS(1000));
    
    // 测试左转
    ESP_LOGI(TAG, "测试左转...");
    motor_left(MOTOR_SPEED_MEDIUM);
    vTaskDelay(pdMS_TO_TICKS(1500));
    
    // 测试停止
    ESP_LOGI(TAG, "测试停止...");
    motor_stop_all();
    vTaskDelay(pdMS_TO_TICKS(1000));
    
    // 测试右转
    ESP_LOGI(TAG, "测试右转...");
    motor_right(MOTOR_SPEED_MEDIUM);
    vTaskDelay(pdMS_TO_TICKS(1500));
    
    // 最终停止
    ESP_LOGI(TAG, "测试完成，停止所有电机");
    motor_stop_all();
    
    return ESP_OK;
}

esp_err_t motor_turn_angle(int angle, int speed)
{
    if (!motor_initialized) {
        ESP_LOGE(TAG, "电机驱动未初始化");
        return ESP_FAIL;
    }
    
    if (angle == 0) {
        return ESP_OK; // 不需要转弯
    }
    
    if (speed < 0) speed = 0;
    if (speed > 100) speed = 100;
    
    // 限制角度范围
    if (angle < -180) angle = -180;
    if (angle > 180) angle = 180;
    
    ESP_LOGI(TAG, "按角度转弯: %d度, 速度: %d%%", angle, speed);
    
    // 计算转弯时间 (简化模型：1度约需要10ms，根据实际机器人调整)
    int turn_time_ms = abs(angle) * 10 * (100.0 / speed); // 速度越慢，时间越长
    
    esp_err_t result;
    if (angle < 0) {
        // 左转
        result = motor_left(speed);
    } else {
        // 右转
        result = motor_right(speed);
    }
    
    if (result == ESP_OK) {
        // 等待转弯完成
        vTaskDelay(pdMS_TO_TICKS(turn_time_ms));
        motor_stop_all();
        ESP_LOGI(TAG, "角度转弯完成");
    }
    
    return result;
}

esp_err_t motor_differential_drive(int left_speed, int right_speed)
{
    if (!motor_initialized) {
        ESP_LOGE(TAG, "电机驱动未初始化");
        return ESP_FAIL;
    }
    
    // 限制速度范围
    if (left_speed < -100) left_speed = -100;
    if (left_speed > 100) left_speed = 100;
    if (right_speed < -100) right_speed = -100;
    if (right_speed > 100) right_speed = 100;
    
    ESP_LOGI(TAG, "差速驱动: 左轮=%d%%, 右轮=%d%%", left_speed, right_speed);
    
    // 控制左轮 (IN1, IN2)
    if (left_speed > 0) {
        // 左轮前进
        gpio_set_level(MOTOR_IN1_GPIO, 1);
        gpio_set_level(MOTOR_IN2_GPIO, 0);
    } else if (left_speed < 0) {
        // 左轮后退
        gpio_set_level(MOTOR_IN1_GPIO, 0);
        gpio_set_level(MOTOR_IN2_GPIO, 1);
    } else {
        // 左轮停止
        gpio_set_level(MOTOR_IN1_GPIO, 0);
        gpio_set_level(MOTOR_IN2_GPIO, 0);
    }
    
    // 控制右轮 (IN3, IN4)
    if (right_speed > 0) {
        // 右轮前进
        gpio_set_level(MOTOR_IN3_GPIO, 1);
        gpio_set_level(MOTOR_IN4_GPIO, 0);
    } else if (right_speed < 0) {
        // 右轮后退
        gpio_set_level(MOTOR_IN3_GPIO, 0);
        gpio_set_level(MOTOR_IN4_GPIO, 1);
    } else {
        // 右轮停止
        gpio_set_level(MOTOR_IN3_GPIO, 0);
        gpio_set_level(MOTOR_IN4_GPIO, 0);
    }
    
    return ESP_OK;
}

esp_err_t motor_pivot_turn(int angle, int speed)
{
    if (!motor_initialized) {
        ESP_LOGE(TAG, "电机驱动未初始化");
        return ESP_FAIL;
    }
    
    if (angle == 0) {
        return ESP_OK; // 不需要转弯
    }
    
    if (speed < 0) speed = 0;
    if (speed > 100) speed = 100;
    
    // 限制角度范围
    if (angle < -180) angle = -180;
    if (angle > 180) angle = 180;
    
    ESP_LOGI(TAG, "原地转弯: %d度, 速度: %d%%", angle, speed);
    
    // 计算转弯时间 (原地转弯比移动转弯快一些)
    int turn_time_ms = abs(angle) * 8 * (100.0 / speed);
    
    esp_err_t result;
    if (angle < 0) {
        // 左转：左轮后退，右轮前进
        result = motor_differential_drive(-speed, speed);
    } else {
        // 右转：左轮前进，右轮后退
        result = motor_differential_drive(speed, -speed);
    }
    
    if (result == ESP_OK) {
        // 等待转弯完成
        vTaskDelay(pdMS_TO_TICKS(turn_time_ms));
        motor_stop_all();
        ESP_LOGI(TAG, "原地转弯完成");
    }
    
    return result;
}