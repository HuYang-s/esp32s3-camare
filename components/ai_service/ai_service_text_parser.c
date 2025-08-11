#include "ai_service.h"
#include "motor_driver.h"
#include "storage_manager.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <string.h>
#include <stdio.h>

static const char *TAG = "ai_text_parser";

// 解析文本中的电机控制指令
esp_err_t parse_text_motor_command(const char* text, const char* filename)
{
    ESP_LOGI(TAG, "🔍 尝试从AI文本响应中解析电机控制指令...");
    ESP_LOGI(TAG, "📝 待解析文本: %.300s", text);
    
    // 检查是否包含control_motor函数调用
    if (strstr(text, "control_motor") == NULL) {
        ESP_LOGW(TAG, "❌ 文本中未找到control_motor函数调用");
        return ESP_FAIL;
    }
    
    ESP_LOGI(TAG, "✅ 发现文本中包含control_motor调用，开始解析参数...");
    
    const char* action_str = NULL;
    double duration_val = 1.0;
    int speed_val = 60;
    
    // 解析动作参数
    if (strstr(text, "'stop'") || strstr(text, "\"stop\"") || strstr(text, "Stop")) {
        action_str = "stop";
    } else if (strstr(text, "'forward'") || strstr(text, "\"forward\"") || strstr(text, "前进")) {
        action_str = "forward";
    } else if (strstr(text, "'backward'") || strstr(text, "\"backward\"") || strstr(text, "后退")) {
        action_str = "backward";
    } else if (strstr(text, "'left'") || strstr(text, "\"left\"") || strstr(text, "左转")) {
        action_str = "left";
    } else if (strstr(text, "'right'") || strstr(text, "\"right\"") || strstr(text, "右转")) {
        action_str = "right";
    }
    
    if (!action_str) {
        ESP_LOGW(TAG, "❌ 无法从文本中识别有效的动作参数");
        return ESP_FAIL;
    }
    
    // 尝试解析duration参数
    const char* duration_str = strstr(text, "duration");
    if (duration_str) {
        // 查找数字
        for (int i = 0; i < 50 && duration_str[i]; i++) {
            if ((duration_str[i] >= '0' && duration_str[i] <= '9') || duration_str[i] == '.') {
                sscanf(&duration_str[i], "%lf", &duration_val);
                break;
            }
        }
    }
    
    // 尝试解析speed参数
    const char* speed_str = strstr(text, "speed");
    if (speed_str) {
        // 查找数字
        for (int i = 0; i < 50 && speed_str[i]; i++) {
            if (speed_str[i] >= '0' && speed_str[i] <= '9') {
                sscanf(&speed_str[i], "%d", &speed_val);
                break;
            }
        }
    }
    
    // 限制参数范围
    if (duration_val < 0.5) duration_val = 0.5;
    if (duration_val > 3.0) duration_val = 3.0;
    if (speed_val < 30) speed_val = 30;
    if (speed_val > 100) speed_val = 100;
    
    ESP_LOGI(TAG, "🔧 从文本解析出的电机控制参数:");
    ESP_LOGI(TAG, "   动作: %s", action_str);
    ESP_LOGI(TAG, "   持续时间: %.1f秒", duration_val);
    ESP_LOGI(TAG, "   速度: %d%%", speed_val);
    
    // 执行电机控制
    esp_err_t motor_result = ESP_FAIL;
    ESP_LOGI(TAG, "🔄 准备执行从文本解析的电机控制: %s", action_str);
    
    if (strcmp(action_str, "forward") == 0) {
        ESP_LOGI(TAG, "⬆️  调用motor_forward函数, 速度: %d%%", speed_val);
        motor_result = motor_forward(speed_val);
        ESP_LOGI(TAG, "⬆️  motor_forward返回结果: %s", motor_result == ESP_OK ? "✅成功" : "❌失败");
    } else if (strcmp(action_str, "backward") == 0) {
        ESP_LOGI(TAG, "⬇️  调用motor_backward函数, 速度: %d%%", speed_val);
        motor_result = motor_backward(speed_val);
        ESP_LOGI(TAG, "⬇️  motor_backward返回结果: %s", motor_result == ESP_OK ? "✅成功" : "❌失败");
    } else if (strcmp(action_str, "left") == 0) {
        ESP_LOGI(TAG, "⬅️  调用motor_left函数, 速度: %d%%", speed_val);
        motor_result = motor_left(speed_val);
        ESP_LOGI(TAG, "⬅️  motor_left返回结果: %s", motor_result == ESP_OK ? "✅成功" : "❌失败");
    } else if (strcmp(action_str, "right") == 0) {
        ESP_LOGI(TAG, "➡️  调用motor_right函数, 速度: %d%%", speed_val);
        motor_result = motor_right(speed_val);
        ESP_LOGI(TAG, "➡️  motor_right返回结果: %s", motor_result == ESP_OK ? "✅成功" : "❌失败");
    } else if (strcmp(action_str, "stop") == 0) {
        ESP_LOGI(TAG, "⏹️  调用motor_stop_all函数");
        motor_result = motor_stop_all();
        ESP_LOGI(TAG, "⏹️  motor_stop_all返回结果: %s", motor_result == ESP_OK ? "✅成功" : "❌失败");
    }
    
    if (motor_result != ESP_OK) {
        ESP_LOGE(TAG, "❌ 从文本解析的电机控制执行失败!");
        return ESP_FAIL;
    }
    
    if (motor_result == ESP_OK && duration_val > 0) {
        ESP_LOGI(TAG, "⏱️  电机开始运行，持续时间: %.1f秒", duration_val);
        // 执行指定持续时间
        vTaskDelay(pdMS_TO_TICKS((int)(duration_val * 1000)));
        ESP_LOGI(TAG, "⏱️  持续时间结束，停止电机");
        motor_stop_all(); // 时间到后停止
        ESP_LOGI(TAG, "🛑 电机已停止");
    }
    
    // 保存AI决策结果
    char ai_decision[256];
    snprintf(ai_decision, sizeof(ai_decision), 
        "AI文本解析驾驶决策: %s (%.1f秒, 速度%d%%)", 
        action_str, duration_val, speed_val);
    storage_manager_update_ai_result(filename, ai_decision);
    
    ESP_LOGI(TAG, "✅ 文本解析电机控制执行完成");
    return ESP_OK;
}