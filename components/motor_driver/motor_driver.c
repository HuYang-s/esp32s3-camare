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