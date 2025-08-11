#include <time.h>
#include <string.h>
#include "esp_log.h"
#include "esp_sntp.h"

#include "time_service.h"

static const char *TAG = "time_service";
static bool time_synchronized = false;

// 时间同步回调函数
static void time_sync_notification_cb(struct timeval *tv)
{
    ESP_LOGI(TAG, "时间同步完成");
    time_synchronized = true;
}

// 内部初始化SNTP时间同步函数
static void initialize_sntp(void)
{
    ESP_LOGI(TAG, "初始化SNTP时间同步");
    
    // 设置时区为北京时间 (UTC+8)
    setenv("TZ", "CST-8", 1);
    tzset();
    
    esp_sntp_setoperatingmode(SNTP_OPMODE_POLL);
    esp_sntp_setservername(0, "ntp.aliyun.com");      // 阿里云NTP服务器
    esp_sntp_setservername(1, "cn.pool.ntp.org");     // 中国NTP服务器池
    esp_sntp_setservername(2, "pool.ntp.org");        // 全球NTP服务器池
    
    esp_sntp_set_time_sync_notification_cb(time_sync_notification_cb);
    esp_sntp_init();
    
    ESP_LOGI(TAG, "SNTP初始化完成，等待时间同步...");
}

// --- Public Functions ---

void time_service_init(void)
{
    initialize_sntp();
}

void time_service_get_beijing_time_string(char* time_str, size_t max_len)
{
    time_t now = 0;
    struct tm timeinfo = { 0 };
    
    time(&now);
    localtime_r(&now, &timeinfo);
    
    if (time_synchronized) {
        strftime(time_str, max_len, "%Y-%m-%d %H:%M:%S", &timeinfo);
    } else {
        snprintf(time_str, max_len, "时间同步中...");
    }
}

bool time_service_is_time_synchronized(void)
{
    return time_synchronized;
}
