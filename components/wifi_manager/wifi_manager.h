#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

/**
 * @brief 初始化WiFi为AP+STA双模式
 */
void wifi_manager_init_ap_sta(void);

/**
 * @brief 等待STA模式连接成功
 *
 * @param timeout 超时时间 (in FreeRTOS ticks)
 * @return esp_err_t ESP_OK if connected, ESP_FAIL on timeout.
 */
esp_err_t wifi_manager_wait_for_connect(TickType_t timeout);

/**
 * @brief 检查STA模式是否已连接
 */
bool wifi_manager_is_sta_connected(void);

/**
 * @brief 获取STA模式的IP地址
 */
void wifi_manager_get_sta_ip(char* ip_buffer, size_t buffer_size);

/**
 * @brief 获取AP模式的IP地址
 */
void wifi_manager_get_ap_ip(char* ip_buffer, size_t buffer_size);

/**
 * @brief 检查AP模式是否已启动
 */
bool wifi_manager_is_ap_started(void);

#endif // WIFI_MANAGER_H