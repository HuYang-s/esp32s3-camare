#include <string.h>
#include "esp_mac.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"

#include "wifi_manager.h"

static const char *TAG = "wifi_manager";

// WiFi STA模式配置
#define WIFI_SSID "bed_room_2.4G"
#define WIFI_PASS "Hdk4560.0"

// WiFi AP模式配置
#define AP_SSID "ESP32-S3-Camera"
#define AP_PASS "12345678"
#define AP_CHANNEL 1
#define AP_MAX_CONN 4

// 信号量，用于同步WiFi连接状态
static SemaphoreHandle_t wifi_connected_semaphore;

// 网络状态变量
static bool sta_connected = false;
static bool ap_started = false;
static char sta_ip[16] = {0};
static char ap_ip[16] = {0};

static void wifi_event_handler(void* arg, esp_event_base_t event_base,
                              int32_t event_id, void* event_data)
{
    (void)arg;
    
    if (event_base == WIFI_EVENT) {
        switch (event_id) {
            case WIFI_EVENT_STA_START:
                ESP_LOGI(TAG, "STA模式启动，开始连接到WiFi网络");
                esp_wifi_connect();
                break;
                
            case WIFI_EVENT_STA_DISCONNECTED:
                ESP_LOGI(TAG, "STA模式断开连接，尝试重新连接...");
                sta_connected = false;
                memset(sta_ip, 0, sizeof(sta_ip));
                esp_wifi_connect();
                break;
                
            case WIFI_EVENT_AP_START:
                ESP_LOGI(TAG, "AP模式启动成功");
                ap_started = true;
                strcpy(ap_ip, "192.168.4.1");  // AP模式默认IP
                break;
                
            case WIFI_EVENT_AP_STOP:
                ESP_LOGI(TAG, "AP模式停止");
                ap_started = false;
                memset(ap_ip, 0, sizeof(ap_ip));
                break;
                
            case WIFI_EVENT_AP_STACONNECTED:
                {
                    wifi_event_ap_staconnected_t* event = (wifi_event_ap_staconnected_t*) event_data;
                    ESP_LOGI(TAG, "设备连接到AP: MAC " MACSTR " AID=%d",
                           MAC2STR(event->mac), event->aid);
                }
                break;
                
            case WIFI_EVENT_AP_STADISCONNECTED:
                {
                    wifi_event_ap_stadisconnected_t* event = (wifi_event_ap_stadisconnected_t*) event_data;
                    ESP_LOGI(TAG, "设备从AP断开: MAC " MACSTR " AID=%d",
                           MAC2STR(event->mac), event->aid);
                }
                break;
                
            default:
                break;
        }
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI(TAG, "STA模式获取IP地址:" IPSTR, IP2STR(&event->ip_info.ip));
        snprintf(sta_ip, sizeof(sta_ip), IPSTR, IP2STR(&event->ip_info.ip));
        sta_connected = true;
        xSemaphoreGive(wifi_connected_semaphore);
    }
}

void wifi_manager_init_ap_sta(void)
{
    wifi_connected_semaphore = xSemaphoreCreateBinary();

    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    
    esp_netif_create_default_wifi_sta();
    esp_netif_create_default_wifi_ap();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    esp_event_handler_instance_t instance_any_id;
    esp_event_handler_instance_t instance_got_ip;
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        &wifi_event_handler,
                                                        NULL,
                                                        &instance_any_id));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,
                                                        IP_EVENT_STA_GOT_IP,
                                                        &wifi_event_handler,
                                                        NULL,
                                                        &instance_got_ip));

    wifi_config_t wifi_sta_config = {
        .sta = {
            .ssid = WIFI_SSID,
            .password = WIFI_PASS,
            .threshold.authmode = WIFI_AUTH_WPA2_PSK,
            .pmf_cfg = {
                .capable = true,
                .required = false
            },
        },
    };
    
    wifi_config_t wifi_ap_config = {
        .ap = {
            .ssid = AP_SSID,
            .ssid_len = strlen(AP_SSID),
            .channel = AP_CHANNEL,
            .password = AP_PASS,
            .max_connection = AP_MAX_CONN,
            .authmode = WIFI_AUTH_WPA2_PSK,
            .pmf_cfg = {
                .required = false,
            },
        },
    };

    if (strlen(AP_PASS) == 0) {
        wifi_ap_config.ap.authmode = WIFI_AUTH_OPEN;
    }

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_APSTA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_sta_config));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_ap_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "WiFi AP+STA模式初始化完成");
    ESP_LOGI(TAG, "AP模式 - SSID:%s, 密码:%s, 频道:%d", AP_SSID, AP_PASS, AP_CHANNEL);
    ESP_LOGI(TAG, "STA模式 - 正在连接到:%s", WIFI_SSID);
}

esp_err_t wifi_manager_wait_for_connect(TickType_t timeout)
{
    if (xSemaphoreTake(wifi_connected_semaphore, timeout) == pdTRUE) {
        return ESP_OK;
    }
    return ESP_FAIL;
}

bool wifi_manager_is_sta_connected(void)
{
    return sta_connected;
}

void wifi_manager_get_sta_ip(char* ip_buffer, size_t buffer_size)
{
    if (sta_connected) {
        strncpy(ip_buffer, sta_ip, buffer_size);
    } else {
        strncpy(ip_buffer, "未连接", buffer_size);
    }
}

void wifi_manager_get_ap_ip(char* ip_buffer, size_t buffer_size)
{
    if (ap_started) {
        strncpy(ip_buffer, ap_ip, buffer_size);
    } else {
        strncpy(ip_buffer, "未启动", buffer_size);
    }
}

bool wifi_manager_is_ap_started(void)
{
    return ap_started;
}
