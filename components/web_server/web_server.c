#include <string.h>
#include <sys/stat.h>
#include <sys/param.h>
#include "esp_http_server.h"
#include "esp_log.h"
#include "cJSON.h"

#include "web_server.h"
#include "wifi_manager.h"
#include "time_service.h"
#include "storage_manager.h"
#include "motor_driver.h"
#include "camera_driver.h"
#include "ai_service.h"
#include "local_ai_service.h"
#include "navigation_service.h"

static const char *TAG = "web_server";

// AI自动驾驶状态
static bool ai_auto_drive_enabled = false;

// WiFi凭证，仅用于在网页上显示SSID
#define WIFI_SSID "bed_room_2.4G"
#define AP_SSID "ESP32-S3-Camera"
#define AP_MAX_CONN 4

// --- HTTP Handlers ---

static esp_err_t network_api_handler(httpd_req_t *req)
{
    cJSON *root = cJSON_CreateObject();
    
    // STA模式信息
    cJSON *sta_info = cJSON_CreateObject();
    bool sta_connected = wifi_manager_is_sta_connected();
    char sta_ip[16];
    wifi_manager_get_sta_ip(sta_ip, sizeof(sta_ip));
    cJSON_AddBoolToObject(sta_info, "connected", sta_connected);
    cJSON_AddStringToObject(sta_info, "ssid", WIFI_SSID);
    cJSON_AddStringToObject(sta_info, "ip", sta_ip);
    cJSON_AddItemToObject(root, "sta", sta_info);
    
    // AP模式信息
    cJSON *ap_info = cJSON_CreateObject();
    bool ap_started = wifi_manager_is_ap_started();
    char ap_ip[16];
    wifi_manager_get_ap_ip(ap_ip, sizeof(ap_ip));
    cJSON_AddBoolToObject(ap_info, "started", ap_started);
    cJSON_AddStringToObject(ap_info, "ssid", AP_SSID);
    cJSON_AddStringToObject(ap_info, "ip", ap_ip);
    cJSON_AddNumberToObject(ap_info, "max_connections", AP_MAX_CONN);
    cJSON_AddItemToObject(root, "ap", ap_info);
    
    char *json_string = cJSON_Print(root);
    httpd_resp_set_type(req, "application/json");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
    httpd_resp_send(req, json_string, strlen(json_string));
    
    free(json_string);
    cJSON_Delete(root);
    return ESP_OK;
}

static esp_err_t time_api_handler(httpd_req_t *req)
{
    char time_str[64];
    time_service_get_beijing_time_string(time_str, sizeof(time_str));
    
    time_t now;
    time(&now);
    
    cJSON *root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "time", time_str);
    cJSON_AddNumberToObject(root, "timestamp", (double)now);
    cJSON_AddBoolToObject(root, "synchronized", time_service_is_time_synchronized());
    
    char *json_string = cJSON_Print(root);
    httpd_resp_set_type(req, "application/json");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
    httpd_resp_send(req, json_string, strlen(json_string));
    
    free(json_string);
    cJSON_Delete(root);
    return ESP_OK;
}

static esp_err_t history_api_handler(httpd_req_t *req)
{
    image_info_t history_buffer[MAX_HISTORY_IMAGES];
    int image_count = storage_manager_get_history(history_buffer, MAX_HISTORY_IMAGES);

    cJSON *root = cJSON_CreateObject();
    cJSON *images = cJSON_CreateArray();

    for (int i = 0; i < image_count; i++) {
        cJSON *image = cJSON_CreateObject();
        
        const char* filename_only = strrchr(history_buffer[i].filename, '/');
        filename_only = filename_only ? filename_only + 1 : history_buffer[i].filename;
        
        cJSON_AddStringToObject(image, "filename", filename_only);
        cJSON_AddNumberToObject(image, "timestamp", (double)history_buffer[i].capture_time);
        cJSON_AddBoolToObject(image, "has_ai_result", history_buffer[i].has_ai_result);
        cJSON_AddStringToObject(image, "ai_description", history_buffer[i].ai_description);
        cJSON_AddItemToArray(images, image);
    }

    cJSON_AddItemToObject(root, "images", images);
    cJSON_AddNumberToObject(root, "total", image_count);

    char *json_string = cJSON_Print(root);
    httpd_resp_set_type(req, "application/json");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
    httpd_resp_send(req, json_string, strlen(json_string));

    free(json_string);
    cJSON_Delete(root);
    return ESP_OK;
}

static esp_err_t image_handler(httpd_req_t *req)
{
    char filename[64];
    if (httpd_req_get_url_query_str(req, filename, sizeof(filename)) != ESP_OK) {
        httpd_resp_send_404(req);
        return ESP_FAIL;
    }
    
    char *file_param = strstr(filename, "file=");
    if (file_param == NULL) {
        httpd_resp_send_404(req);
        return ESP_FAIL;
    }
    
    char *actual_filename = file_param + 5; // Skip "file="
    
    char *end = strchr(actual_filename, '&');
    if (end) *end = '\0';
    
    char filepath[128];
    snprintf(filepath, sizeof(filepath), "/spiffs/%s", actual_filename);
    
    struct stat file_stat;
    if (stat(filepath, &file_stat) == -1) {
        ESP_LOGE(TAG, "File not found: %s", filepath);
        httpd_resp_send_404(req);
        return ESP_FAIL;
    }
    
    FILE *file = fopen(filepath, "rb");
    if (file == NULL) {
        ESP_LOGE(TAG, "Failed to open file: %s", filepath);
        httpd_resp_send_404(req);
        return ESP_FAIL;
    }
    
    httpd_resp_set_type(req, "image/jpeg");
    
    char buffer[1024];
    size_t bytes_read;
    while ((bytes_read = fread(buffer, 1, sizeof(buffer), file)) > 0) {
        if (httpd_resp_send_chunk(req, buffer, bytes_read) != ESP_OK) {
            fclose(file);
            return ESP_FAIL;
        }
    }
    
    fclose(file);
    httpd_resp_send_chunk(req, NULL, 0);
    return ESP_OK;
}

static esp_err_t motor_control_handler(httpd_req_t *req)
{
    ESP_LOGI(TAG, "收到电机控制API请求");
    char content[128];
    size_t recv_size = MIN(req->content_len, sizeof(content) - 1);
    
    int ret = httpd_req_recv(req, content, recv_size);
    if (ret <= 0) {
        ESP_LOGE(TAG, "接收HTTP请求数据失败");
        httpd_resp_send_500(req);
        return ESP_FAIL;
    }
    content[ret] = '\0';
    ESP_LOGI(TAG, "接收到的JSON数据: %s", content);
    
    cJSON *json = cJSON_Parse(content);
    if (json == NULL) {
        ESP_LOGE(TAG, "JSON解析失败");
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Invalid JSON");
        return ESP_FAIL;
    }
    
    cJSON *action = cJSON_GetObjectItem(json, "action");
    cJSON *speed = cJSON_GetObjectItem(json, "speed");
    
    if (!cJSON_IsString(action)) {
        ESP_LOGE(TAG, "action字段不是字符串或不存在");
        cJSON_Delete(json);
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Invalid JSON");
        return ESP_FAIL;
    }
    
    int motor_speed = 50; // 默认速度
    if (cJSON_IsNumber(speed)) {
        motor_speed = (int)cJSON_GetNumberValue(speed);
        if (motor_speed < 0) motor_speed = 0;
        if (motor_speed > 100) motor_speed = 100;
    }
    
    esp_err_t result = ESP_FAIL;
    const char *action_str = cJSON_GetStringValue(action);
    
    // 复制action字符串，避免在删除JSON对象后使用悬空指针
    char action_copy[32];
    strncpy(action_copy, action_str, sizeof(action_copy) - 1);
    action_copy[sizeof(action_copy) - 1] = '\0';
    
    ESP_LOGI(TAG, "电机控制命令: %s, 速度: %d", action_copy, motor_speed);
    
    if (strcmp(action_copy, "forward") == 0) {
        ESP_LOGI(TAG, "调用motor_forward函数");
        result = motor_forward(motor_speed);
        ESP_LOGI(TAG, "motor_forward返回结果: %s", result == ESP_OK ? "成功" : "失败");
    } else if (strcmp(action_copy, "backward") == 0) {
        result = motor_backward(motor_speed);
    } else if (strcmp(action_copy, "left") == 0) {
        result = motor_left(motor_speed);
    } else if (strcmp(action_copy, "right") == 0) {
        result = motor_right(motor_speed);
    } else if (strcmp(action_copy, "stop") == 0) {
        result = motor_stop_all();
    }
    
    cJSON_Delete(json);
    
    cJSON *response = cJSON_CreateObject();
    cJSON_AddBoolToObject(response, "success", result == ESP_OK);
    cJSON_AddStringToObject(response, "action", action_copy);
    cJSON_AddNumberToObject(response, "speed", motor_speed);
    
    char *json_string = cJSON_Print(response);
    httpd_resp_set_type(req, "application/json");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
    httpd_resp_send(req, json_string, strlen(json_string));
    
    free(json_string);
    cJSON_Delete(response);
    
    return result == ESP_OK ? ESP_OK : ESP_FAIL;
}

// AI自动驾驶状态API处理函数
static esp_err_t auto_drive_api_handler(httpd_req_t *req)
{
    if (req->method == HTTP_GET) {
        // 获取AI自动驾驶状态
        cJSON *response = cJSON_CreateObject();
        cJSON_AddBoolToObject(response, "enabled", ai_auto_drive_enabled);
        
        char *json_string = cJSON_Print(response);
        httpd_resp_set_type(req, "application/json");
        httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
        httpd_resp_send(req, json_string, strlen(json_string));
        
        free(json_string);
        cJSON_Delete(response);
        return ESP_OK;
    }
    else if (req->method == HTTP_POST) {
        // 设置AI自动驾驶状态
        char content[128];
        size_t recv_size = MIN(req->content_len, sizeof(content) - 1);
        
        int ret = httpd_req_recv(req, content, recv_size);
        if (ret <= 0) {
            httpd_resp_send_500(req);
            return ESP_FAIL;
        }
        content[ret] = '\0';
        
        cJSON *json = cJSON_Parse(content);
        if (json == NULL) {
            httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Invalid JSON");
            return ESP_FAIL;
        }
        
        cJSON *enabled = cJSON_GetObjectItem(json, "enabled");
        if (!cJSON_IsBool(enabled)) {
            cJSON_Delete(json);
            httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Invalid JSON");
            return ESP_FAIL;
        }
        
        ai_auto_drive_enabled = cJSON_IsTrue(enabled);
        ESP_LOGI(TAG, "AI自动驾驶状态: %s", ai_auto_drive_enabled ? "开启" : "关闭");
        
        cJSON *response = cJSON_CreateObject();
        cJSON_AddBoolToObject(response, "success", true);
        cJSON_AddBoolToObject(response, "enabled", ai_auto_drive_enabled);
        
        char *json_string = cJSON_Print(response);
        httpd_resp_set_type(req, "application/json");
        httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
        httpd_resp_send(req, json_string, strlen(json_string));
        
        free(json_string);
        cJSON_Delete(response);
        cJSON_Delete(json);
        return ESP_OK;
    }
    
    httpd_resp_send_404(req);
    return ESP_FAIL;
}



static esp_err_t index_handler(httpd_req_t *req)
{
    const char* html = "<!DOCTYPE html>"
        "<html><head>"
        "<title>ESP32-S3摄像头AI分析系统</title>"
        "<meta charset='UTF-8'>"
        "<meta name='viewport' content='width=device-width, initial-scale=1.0'>"
        "<style>"
        "body{font-family:Arial,sans-serif;margin:8px;background-color:#f5f5f5;font-size:13px}"
        ".container{max-width:1400px;margin:0 auto;background-color:white;padding:10px;border-radius:6px;box-shadow:0 1px 4px rgba(0,0,0,0.1)}"
        ".header{text-align:center;margin-bottom:15px}"
        ".status{display:grid;grid-template-columns:repeat(auto-fit,minmax(140px,1fr));gap:6px;margin:10px 0}"
        ".status-item{background:#f8f9fa;padding:5px 8px;border-radius:4px;font-size:11px;text-align:center}"
        ".controls{text-align:center;margin:8px 0}"
        ".btn{background:#007bff;color:white;border:none;padding:5px 10px;border-radius:4px;cursor:pointer;margin:3px;font-size:11px}"
        ".btn:hover{background:#0056b3}"
        ".btn:disabled{background:#ccc;cursor:not-allowed}"
        ".motor-controls{background:#f8f9fa;padding:10px;border-radius:6px;margin:10px 0;text-align:center;max-width:280px;margin-left:auto;margin-right:auto}"
        ".motor-btn{background:#28a745;color:white;border:none;padding:6px;border-radius:4px;cursor:pointer;font-size:11px;font-weight:bold;width:50px;height:30px;transition:all 0.2s}"
        ".motor-btn:hover{background:#218838}"
        ".motor-btn:active{background:#1e7e34;transform:translateY(1px)}"
        ".motor-btn.stop{background:#dc3545;width:50px;grid-column:2;grid-row:2}"
        ".motor-btn.stop:hover{background:#c82333}"
        ".speed-control{margin:8px 0}"
        ".speed-slider{width:120px;margin:0 5px}"
        ".motor-grid{display:grid;grid-template-columns:1fr 1fr 1fr;grid-template-rows:1fr 1fr 1fr;gap:5px;max-width:180px;margin:0 auto}"
        ".motor-btn.forward{grid-column:2;grid-row:1}"
        ".motor-btn.left{grid-column:1;grid-row:2}"
        ".motor-btn.right{grid-column:3;grid-row:2}"
        ".motor-btn.backward{grid-column:2;grid-row:3}"
        ".grid{display:grid;grid-template-columns:repeat(auto-fill,minmax(250px,1fr));gap:8px;margin-top:10px}"
        ".card{border:1px solid #ddd;border-radius:6px;overflow:hidden;box-shadow:0 1px 3px rgba(0,0,0,0.1)}"
        ".card img{width:100%;height:140px;object-fit:cover}"
        ".card-content{padding:8px}"
        ".timestamp{color:#666;font-size:10px;margin-bottom:5px}"
        ".ai-result{background:#e3f2fd;padding:5px;border-radius:3px;margin:3px 0;font-size:11px;line-height:1.3;white-space:pre-wrap}"
        ".ai-motor-control{background:#fff3cd;padding:8px;border-radius:3px;margin:3px 0;font-size:11px;line-height:1.4;border-left:3px solid #f0ad4e;white-space:pre-wrap}"
        ".ai-thinking{background:#f8f9fa;border-left:3px solid #6f42c1;padding:6px;margin:2px 0;font-size:10px;line-height:1.3;color:#5a5a5a;white-space:pre-wrap}"
        ".ai-decision{background:#d1ecf1;border-left:3px solid #17a2b8;padding:5px;margin:2px 0;font-size:11px;line-height:1.3;font-weight:bold}"
        ".loading{text-align:center;padding:15px;color:#666;font-size:12px}"
        ".error{color:red;text-align:center;padding:15px;font-size:12px}"
        ".sync-status{display:inline-block;margin-left:10px;font-size:10px}"
        ".sync-ok{color:green}.sync-error{color:red}"
        ".motor-status{margin:5px 0;font-size:10px;color:#666;text-align:center;min-height:12px}"
        ".main-content{display:grid;grid-template-columns:300px 1fr;gap:15px;margin:10px 0}"
        ".sidebar{display:flex;flex-direction:column;gap:10px}"
        ".images-section{min-height:300px}"
        "h1{font-size:18px;margin:5px 0}"
        "h3{font-size:14px;margin:5px 0}"
        ".ai-auto-drive{background:#e8f5e8;padding:10px;border-radius:6px;margin-top:10px}"
        ".auto-drive-toggle{display:flex;align-items:center;cursor:pointer;font-size:11px;margin:5px 0}"
        ".auto-drive-toggle input[type=checkbox]{display:none}"
        ".toggle-slider{width:30px;height:15px;background:#ccc;border-radius:15px;position:relative;transition:0.3s;margin-right:8px}"
        ".toggle-slider:before{content:'';position:absolute;width:11px;height:11px;border-radius:50%;background:white;top:2px;left:2px;transition:0.3s}"
        ".auto-drive-toggle input:checked + .toggle-slider{background:#4CAF50}"
        ".auto-drive-toggle input:checked + .toggle-slider:before{transform:translateX(15px)}"
        ".auto-drive-status{font-size:10px;color:#666;margin:3px 0;font-weight:bold}"
        ".auto-drive-info{font-size:9px;color:#888;margin-top:5px;line-height:1.3}"

        ".command-response{background:#ffffff;border:1px solid #e0e0e0;border-radius:4px;padding:8px;margin:5px 0;font-size:11px;line-height:1.4;min-height:20px;max-height:150px;overflow-y:auto}"
        ".command-response.loading{color:#666;font-style:italic}"
        ".command-response.success{border-left:3px solid #28a745}"
        ".command-response.error{border-left:3px solid #dc3545;color:#721c24}"
        ".ai-task-control{background:#f0fff0;padding:10px;border-radius:6px;margin-top:10px}"
        ".task-input-area{margin:8px 0}"
        ".task-input{width:100%;padding:6px;border:1px solid #ccc;border-radius:4px;font-size:11px;margin:3px 0}"
        ".task-controls{display:flex;gap:5px;margin-top:5px;justify-content:center}"
        ".task-btn{background:#ff6b35;padding:4px 8px;font-size:10px}"
        ".task-btn:hover{background:#e55a2b}"
        ".task-status{font-size:10px;color:#666;margin:5px 0;text-align:center;font-weight:bold}"
        ".task-result{background:#ffffff;border:1px solid #e0e0e0;border-radius:4px;padding:8px;margin:5px 0;font-size:11px;line-height:1.4;min-height:20px;max-height:150px;overflow-y:auto}"
        ".task-result.searching{border-left:3px solid #ffc107;color:#856404}"
        ".task-result.completed{border-left:3px solid #28a745;color:#155724}"
        ".task-result.failed{border-left:3px solid #dc3545;color:#721c24}"
        ".checkbox-label{display:block;margin:8px 0;font-size:12px;color:#666}"
        ".checkbox-label input{margin-right:5px}"
        ".task-progress{margin:8px 0;padding:8px;background:#f5f5f5;border-radius:4px}"
        ".progress-bar{width:100%;height:20px;background:#e0e0e0;border-radius:10px;overflow:hidden;margin-bottom:5px}"
        ".progress-fill{height:100%;background:linear-gradient(90deg,#4CAF50,#45a049);transition:width 0.3s ease;border-radius:10px}"
        ".progress-text{font-size:11px;color:#666;font-weight:bold}"
        ".task-log{margin:10px 0;padding:10px;background:#f0f8ff;border:1px solid #d0d0d0;border-radius:6px;max-height:200px;overflow-y:auto}"
        ".log-content{font-family:monospace;font-size:10px;line-height:1.4;white-space:pre-wrap;color:#333;margin-bottom:8px}"
        ".btn.info{background:#17a2b8;border-color:#17a2b8}"
        ".btn.info:hover{background:#138496;border-color:#117a8b}"
        "@media (max-width:900px){.main-content{grid-template-columns:1fr;gap:10px}.motor-controls{max-width:280px;margin:0 auto}}"
        "@media (max-width:600px){.status{grid-template-columns:1fr 1fr}.grid{grid-template-columns:1fr}.motor-grid{max-width:160px}.motor-btn{width:45px;height:25px;font-size:10px}}"
        "</style>"
        "</head><body>"
        "<div class='container'>"
        "<div class='header'>"
        "<h1>ESP32-S3摄像头AI分析系统</h1>"
        "<div>当前时间: <span id='current-time'>加载中...</span><span id='sync-status' class='sync-status'></span></div>"
        "</div>"
        "<div class='status' id='status'>加载中...</div>"
        "<div class='controls'>"
        "<button class='btn' onclick='loadImages()'>刷新图片</button>"
        "<button class='btn' onclick='updateTime()'>更新时间</button>"
        "<label><input type='checkbox' id='auto-refresh' checked> 自动刷新 <span id='refresh-interval'>(6秒)</span></label>"
        "</div>"
        "<div class='main-content'>"
        "<div class='sidebar'>"
        "<div class='motor-controls'>"
        "<h3>🚗 L298N电机控制</h3>"
        "<div class='motor-grid'>"
        "<button class='motor-btn forward' onmousedown='startMotor(\"forward\")' onmouseup='stopMotor()' ontouchstart='startMotor(\"forward\")' ontouchend='stopMotor()'>▲<br>前进</button>"
        "<button class='motor-btn left' onmousedown='startMotor(\"left\")' onmouseup='stopMotor()' ontouchstart='startMotor(\"left\")' ontouchend='stopMotor()'>◄<br>左转</button>"
        "<button class='motor-btn stop' onclick='stopMotor()'>⏹<br>停止</button>"
        "<button class='motor-btn right' onmousedown='startMotor(\"right\")' onmouseup='stopMotor()' ontouchstart='startMotor(\"right\")' ontouchend='stopMotor()'>►<br>右转</button>"
        "<button class='motor-btn backward' onmousedown='startMotor(\"backward\")' onmouseup='stopMotor()' ontouchstart='startMotor(\"backward\")' ontouchend='stopMotor()'>▼<br>后退</button>"
        "</div>"
        "<div class='speed-control'>"
        "<label>速度: <input type='range' id='speed-slider' class='speed-slider' min='30' max='100' value='50' oninput='updateSpeedDisplay()'> <span id='speed-value'>50</span>%</label>"
        "</div>"
        "<div class='motor-status' id='motor-status'>电机状态: 停止</div>"
        "</div>"
        "<div class='ai-auto-drive'>"
        "<h3>🤖 AI自动驾驶</h3>"
        "<label class='auto-drive-toggle'>"
        "<input type='checkbox' id='ai-auto-drive' onchange='toggleAutoDrive()'>"
        "<span class='toggle-slider'></span>"
        "启用AI自动驾驶"
        "</label>"
        "<div class='auto-drive-status' id='auto-drive-status'>AI自动驾驶: 关闭</div>"
        "<div class='auto-drive-info'>AI将根据摄像头图像自动控制电机移动</div>"
        "</div>"

        "<div class='ai-task-control'>"
        "<h3>🎯 AI物体搜索任务</h3>"
        "<div class='task-input-area'>"
        "<label>目标物品:</label>"
        "<input type='text' id='target-object' class='task-input' placeholder='例如: 杯子, 苹果, 手机...' />"
        "<label>任务超时时间 (秒):</label>"
        "<input type='number' id='task-timeout' class='task-input' value='30' min='10' max='300' />"
        "<label class='checkbox-label'>"
        "<input type='checkbox' id='use-navigation' checked> 启用主动搜索导航"
        "</label>"
        "<div class='task-controls'>"
        "<button class='btn task-btn' onclick='startAINavigationTask()'>🚀 开始搜索</button>"
        "<button class='btn task-btn' onclick='stopAITask()'>⏹️ 停止任务</button>"
        "<button class='btn task-btn info' onclick='showTaskLog()'>📋 查看日志</button>"
        "</div>"
        "</div>"
        "<div class='task-status' id='task-status'>等待任务...</div>"
        "<div class='task-progress'>"
        "<div class='progress-bar'>"
        "<div class='progress-fill' id='progress-fill' style='width: 0%'></div>"
        "</div>"
        "<span class='progress-text' id='progress-text'>0%</span>"
        "</div>"
        "<div class='task-result' id='task-result'></div>"
        "<div class='task-log' id='task-log' style='display:none'>"
        "<h4>📋 任务详细日志</h4>"
        "<div class='log-content' id='log-content'></div>"
        "<button class='btn' onclick='hideTaskLog()'>关闭日志</button>"
        "</div>"
        "</div>"
        "</div>"
        "<div class='images-section'>"
        "<h3>📸 图片历史</h3>"
        "<div id='images' class='grid'>加载中...</div>"
        "</div>"
        "</div>"
        "</div>"
        "<script>"
        "let refreshInterval;let isPageVisible=true;let autoRefreshEnabled=true;let currentMotorAction='';"
        "document.addEventListener('visibilitychange',()=>{"
        "isPageVisible=!document.hidden;"
        "if(autoRefreshEnabled){updateRefreshInterval()}"
        "});"
        "function updateSpeedDisplay(){"
        "const slider=document.getElementById('speed-slider');"
        "document.getElementById('speed-value').textContent=slider.value"
        "}"
        "function controlMotor(action,speed){"
        "console.log('controlMotor被调用，action:', action, 'speed:', speed);"
        "fetch('/api/motor',{"
        "method:'POST',"
        "headers:{'Content-Type':'application/json'},"
        "body:JSON.stringify({action:action,speed:speed})"
        "}).then(r=>{"
        "console.log('收到响应，状态:', r.status);"
        "return r.json();"
        "}).then(data=>{"
        "console.log('响应数据:', data);"
        "if(data.success){"
        "if(action==='stop'){"
        "document.getElementById('motor-status').textContent='电机状态: 停止'"
        "}else{"
        "document.getElementById('motor-status').textContent=`电机状态: ${getActionName(action)} (速度: ${data.speed}%)`"
        "}"
        "}else{"
        "console.log('电机控制失败');"
        "document.getElementById('motor-status').textContent='电机状态: 控制失败'"
        "}"
        "}).catch(e=>{"
        "console.log('Motor control error:',e);"
        "document.getElementById('motor-status').textContent='电机状态: 网络错误'"
        "})"
        "}"
        "function formatMotorControlResult(description){"
        "if(!description) return '<div class=\"ai-motor-control\">AI分析中...</div>';"
        "let html='';"
        "const sections=description.split('\\n\\n');"
        "for(let section of sections){"
        "section=section.trim();"
        "if(!section) continue;"
        "if(section.startsWith('🧠 AI思考过程:')){"
        "const thinkingContent=section.substring('🧠 AI思考过程:'.length).trim();"
        "html+=`<div class='ai-thinking'><strong>🧠 AI思考过程:</strong><br>${thinkingContent.replace(/\\n/g,'<br>')}</div>`;"
        "}else if(section.startsWith('🚗 驾驶决策:')||section.startsWith('🚗 AI自动驾驶决策:')){"
        "const cleanSection=section.replace(/^🚗\\s*(AI自动)?驾驶决策:\\s*/,'');"
        "html+=`<div class='ai-decision'><strong>🚗 AI驾驶决策:</strong><br>${cleanSection.replace(/\\n/g,'<br>')}</div>`;"
        "}else{"
        "html+=`<div class='ai-motor-control'>${section.replace(/\\n/g,'<br>')}</div>`;"
        "}"
        "}"
        "return html||`<div class='ai-motor-control'>${description.replace(/\\n/g,'<br>')}</div>`;"
        "}"
        "function getActionName(action){"
        "const names={'forward':'前进','backward':'后退','left':'左转','right':'右转','stop':'停止'};"
        "return names[action]||action"
        "}"
        "function startMotor(action){"
        "console.log('startMotor被调用，action:', action);"
        "const speed=parseInt(document.getElementById('speed-slider').value);"
        "console.log('当前速度值:', speed);"
        "currentMotorAction=action;"
        "controlMotor(action,speed)"
        "}"
        "function stopMotor(){"
        "console.log('stopMotor被调用');"
        "currentMotorAction='';"
        "controlMotor('stop',0)"
        "}"
        "function toggleAutoDrive(){"
        "const checkbox=document.getElementById('ai-auto-drive');"
        "const status=document.getElementById('auto-drive-status');"
        "fetch('/api/auto-drive',{"
        "method:'POST',"
        "headers:{'Content-Type':'application/json'},"
        "body:JSON.stringify({enabled:checkbox.checked})"
        "}).then(r=>r.json()).then(data=>{"
        "if(data.success){"
        "if(data.enabled){"
        "status.textContent='AI自动驾驶: 开启';"
        "status.style.color='#4CAF50';"
        "console.log('AI自动驾驶已开启');"
        "}else{"
        "status.textContent='AI自动驾驶: 关闭';"
        "status.style.color='#666';"
        "console.log('AI自动驾驶已关闭');"
        "}"
        "}else{"
        "console.log('AI自动驾驶状态更新失败');"
        "checkbox.checked=!checkbox.checked;"
        "}"
        "}).catch(e=>{"
        "console.log('AI自动驾驶API错误:',e);"
        "checkbox.checked=!checkbox.checked;"
        "})"
        "}"

        "function startAITask(){"
        "const targetObject=document.getElementById('target-object').value.trim();"
        "const timeout=parseInt(document.getElementById('task-timeout').value);"
        "if(!targetObject){"
        "alert('请输入目标物品名称');"
        "return"
        "}"
        "if(timeout<10||timeout>300){"
        "alert('超时时间必须在10-300秒之间');"
        "return"
        "}"
        "const statusDiv=document.getElementById('task-status');"
        "const resultDiv=document.getElementById('task-result');"
        "statusDiv.textContent='🎯 正在启动AI搜索任务...';"
        "statusDiv.style.color='#007bff';"
        "resultDiv.textContent='正在初始化搜索任务，请稍候...';"
        "resultDiv.className='task-result searching';"
        "fetch('/api/ai-task',{"
        "method:'POST',"
        "headers:{'Content-Type':'application/json'},"
        "body:JSON.stringify({action:'start',target:targetObject,timeout:timeout})"
        "}).then(r=>r.json()).then(data=>{"
        "if(data.success){"
        "statusDiv.textContent='✅ AI搜索任务已启动';"
        "statusDiv.style.color='#28a745';"
        "resultDiv.textContent=`正在搜索: ${targetObject} (超时: ${timeout}秒)`;"
        "resultDiv.className='task-result searching';"
        "startTaskStatusPolling();"
        "}else{"
        "statusDiv.textContent='❌ 任务启动失败';"
        "statusDiv.style.color='#dc3545';"
        "resultDiv.textContent=data.error||'未知错误';"
        "resultDiv.className='task-result failed';"
        "}"
        "}).catch(e=>{"
        "console.error('AI任务API错误:',e);"
        "statusDiv.textContent='❌ 网络错误';"
        "statusDiv.style.color='#dc3545';"
        "resultDiv.textContent='网络连接失败，请检查ESP32连接状态';"
        "resultDiv.className='task-result failed';"
        "})"
        "}"
        "function stopAITask(){"
        "const statusDiv=document.getElementById('task-status');"
        "const resultDiv=document.getElementById('task-result');"
        "fetch('/api/ai-task',{"
        "method:'POST',"
        "headers:{'Content-Type':'application/json'},"
        "body:JSON.stringify({action:'stop'})"
        "}).then(r=>r.json()).then(data=>{"
        "if(data.success){"
        "statusDiv.textContent='⏹️ AI搜索任务已停止';"
        "statusDiv.style.color='#666';"
        "resultDiv.textContent='任务已停止';"
        "resultDiv.className='task-result';"
        "stopTaskStatusPolling();"
        "}else{"
        "statusDiv.textContent='❌ 停止任务失败';"
        "statusDiv.style.color='#dc3545';"
        "}"
        "}).catch(e=>{"
        "console.error('停止AI任务错误:',e);"
        "})"
        "}"
        "let taskPollingInterval;"
        "function startTaskStatusPolling(){"
        "taskPollingInterval=setInterval(updateTaskStatus,2000);"
        "}"
        "function stopTaskStatusPolling(){"
        "if(taskPollingInterval){"
        "clearInterval(taskPollingInterval);"
        "taskPollingInterval=null;"
        "}"
        "}"
        "function updateTaskStatus(){"
        "fetch('/api/ai-task-status').then(r=>r.json()).then(data=>{"
        "const statusDiv=document.getElementById('task-status');"
        "const resultDiv=document.getElementById('task-result');"
        "if(data.status==='completed'){"
        "statusDiv.textContent='🎉 搜索任务完成!';"
        "statusDiv.style.color='#28a745';"
        "let resultText=data.message||'找到目标物体!';"
        "if(data.results&&data.results.length>0){"
        "resultText+='\\n\\n检测结果:';"
        "data.results.forEach((result,index)=>{"
        "resultText+=`\\n${index+1}. ${result.class_name} (置信度: ${(result.confidence*100).toFixed(1)}%)`;"
        "});"
        "}"
        "resultDiv.textContent=resultText;"
        "resultDiv.className='task-result completed';"
        "stopTaskStatusPolling();"
        "}else if(data.status==='failed_timeout'){"
        "statusDiv.textContent='⏰ 搜索任务超时';"
        "statusDiv.style.color='#dc3545';"
        "resultDiv.textContent='任务超时，未找到目标物体';"
        "resultDiv.className='task-result failed';"
        "stopTaskStatusPolling();"
        "}else if(data.status==='failed_unable'){"
        "statusDiv.textContent='❌ 搜索任务失败';"
        "statusDiv.style.color='#dc3545';"
        "resultDiv.textContent='无法完成搜索任务';"
        "resultDiv.className='task-result failed';"
        "stopTaskStatusPolling();"
        "}else if(data.status==='searching'){"
        "statusDiv.textContent='🔍 正在搜索中...';"
        "statusDiv.style.color='#ffc107';"
        "let searchText=data.message||'搜索进行中...';"
        "if(data.results&&data.results.length>0){"
        "searchText+='\\n\\n当前检测到:';"
        "data.results.forEach((result,index)=>{"
        "searchText+=`\\n- ${result.class_name} (${(result.confidence*100).toFixed(1)}%)`;"
        "});"
        "}"
        "resultDiv.textContent=searchText;"
        "resultDiv.className='task-result searching';"
        "}"
        "}).catch(e=>console.error('任务状态更新错误:',e))"
        "}"
        "document.addEventListener('keydown',(e)=>{"
        "if(document.activeElement.tagName==='INPUT')return;"
        "const speed=parseInt(document.getElementById('speed-slider').value);"
        "switch(e.key){"
        "case 'ArrowUp':case 'w':case 'W':e.preventDefault();if(currentMotorAction!=='forward'){startMotor('forward')};break;"
        "case 'ArrowDown':case 's':case 'S':e.preventDefault();if(currentMotorAction!=='backward'){startMotor('backward')};break;"
        "case 'ArrowLeft':case 'a':case 'A':e.preventDefault();if(currentMotorAction!=='left'){startMotor('left')};break;"
        "case 'ArrowRight':case 'd':case 'D':e.preventDefault();if(currentMotorAction!=='right'){startMotor('right')};break;"
        "case ' ':e.preventDefault();stopMotor();break"
        "}"
        "});"
        "document.addEventListener('keyup',(e)=>{"
        "if(['ArrowUp','ArrowDown','ArrowLeft','ArrowRight','w','W','s','S','a','A','d','D'].includes(e.key)){"
        "stopMotor()"
        "}"
        "});"
        "if(autoRefreshEnabled){updateRefreshInterval()}"
        "function updateRefreshInterval(){"
        "clearInterval(refreshInterval);"
        "if(autoRefreshEnabled){"
        "const interval=isPageVisible?6000:15000;"
        "document.getElementById('refresh-interval').textContent=`(${interval/1000}秒)`;"
        "refreshInterval=setInterval(()=>{loadImages();updateTime()},interval)"
        "}"
        "}"
        "document.getElementById('auto-refresh').addEventListener('change',(e)=>{"
        "autoRefreshEnabled=e.target.checked;"
        "if(autoRefreshEnabled){updateRefreshInterval()}else{clearInterval(refreshInterval);document.getElementById('refresh-interval').textContent=''}"
        "});"
        "function updateTime(){"
        "fetch('/api/time').then(r=>r.json()).then(data=>{"
        "document.getElementById('current-time').textContent=data.time;"
        "const syncStatus=document.getElementById('sync-status');"
        "if(data.synchronized){"
        "syncStatus.textContent='✓ 已同步';syncStatus.className='sync-status sync-ok'"
        "}else{"
        "syncStatus.textContent='⚠ 未同步';syncStatus.className='sync-status sync-error'"
        "}"
        "}).catch(e=>console.log('Time update error:',e))"
        "}"
        "function updateNetworkStatus(){"
        "fetch('/api/network').then(r=>r.json()).then(data=>{"
        "const statusDiv=document.getElementById('status');"
        "statusDiv.innerHTML=`"
        "<div class='status-item'><strong>STA模式</strong><br>状态: ${data.sta.connected?'已连接':'未连接'}<br>SSID: ${data.sta.ssid}<br>IP: ${data.sta.ip}</div>"
        "<div class='status-item'><strong>AP模式</strong><br>状态: ${data.ap.started?'已启动':'未启动'}<br>SSID: ${data.ap.ssid}<br>IP: ${data.ap.ip}</div>`"
        "}).catch(e=>document.getElementById('status').innerHTML='<div class=\"error\">网络状态获取失败</div>')"
        "}"
        "function loadImages(){"
        "const imagesDiv=document.getElementById('images');"
        "imagesDiv.innerHTML='<div class=\"loading\">加载中...</div>';"
        "fetch('/api/history').then(r=>r.json()).then(data=>{"
        "if(data.images&&data.images.length>0){"
        "imagesDiv.innerHTML=data.images.map(img=>{"
        "const date=new Date(img.timestamp*1000);"
        "const timeStr=date.toLocaleString('zh-CN');"
        "const isMotorControl=img.ai_description&&(img.ai_description.includes('🧠 AI思考过程:')||img.ai_description.includes('🚗 驾驶决策:')||img.ai_description.includes('🚗 AI自动驾驶决策:'));"
        "let aiResultHtml='';"
        "if(img.has_ai_result){"
        "if(isMotorControl){"
        "aiResultHtml=formatMotorControlResult(img.ai_description);"
        "}else{"
        "aiResultHtml=`<div class='ai-result'><strong>AI观察:</strong><br>${img.ai_description}</div>`;"
        "}"
        "}else{"
        "aiResultHtml='<div class=\"ai-result\">AI分析中...</div>';"
        "}"
        "return `<div class='card'>"
        "<img src='/image?file=${img.filename}' alt='${img.filename}' loading='lazy'>"
        "<div class='card-content'>"
        "<div class='timestamp'>拍摄时间: ${timeStr}</div>"
        "<div class='timestamp'>文件名: ${img.filename}</div>"
        "${aiResultHtml}"
        "</div></div>`"
        "}).join('')"
        "}else{"
        "imagesDiv.innerHTML='<div class=\"loading\">暂无图片</div>'"
        "}"
        "}).catch(e=>imagesDiv.innerHTML='<div class=\"error\">图片加载失败</div>')"
        "}"
        "updateTime();updateNetworkStatus();loadImages();updateRefreshInterval();"
        "function startAINavigationTask(){"
        "const targetObj=document.getElementById('target-object').value;"
        "const timeout=parseInt(document.getElementById('task-timeout').value);"
        "const useNavigation=document.getElementById('use-navigation').checked;"
        "if(!targetObj.trim()){alert('请输入目标物品名称');return}"
        "const statusDiv=document.getElementById('task-status');"
        "const resultDiv=document.getElementById('task-result');"
        "const progressFill=document.getElementById('progress-fill');"
        "const progressText=document.getElementById('progress-text');"
        "statusDiv.textContent='🚀 启动AI导航搜索任务...';"
        "statusDiv.style.color='#007bff';"
        "resultDiv.textContent='';"
        "resultDiv.className='task-result';"
        "progressFill.style.width='0%';"
        "progressText.textContent='0%';"
        "fetch('/api/ai-navigation-task',{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify({action:'start',target_object:targetObj,timeout:timeout,use_navigation:useNavigation})})"
        ".then(r=>r.json()).then(data=>{"
        "if(data.status==='success'){"
        "statusDiv.textContent='✅ 任务启动成功，开始搜索: '+targetObj;"
        "statusDiv.style.color='#28a745';"
        "startTaskStatusPolling();"
        "}else{"
        "statusDiv.textContent='❌ 任务启动失败: '+data.message;"
        "statusDiv.style.color='#dc3545';"
        "}"
        "}).catch(e=>{statusDiv.textContent='❌ 网络错误: '+e.message;statusDiv.style.color='#dc3545'})"
        "}"
        "function showTaskLog(){"
        "const logDiv=document.getElementById('task-log');"
        "const logContent=document.getElementById('log-content');"
        "logDiv.style.display='block';"
        "fetch('/api/ai-task-log').then(r=>r.json()).then(data=>{"
        "if(data.status==='success'){"
        "logContent.textContent=data.log||'暂无日志信息';"
        "logDiv.scrollTop=logDiv.scrollHeight;"
        "}else{"
        "logContent.textContent='获取日志失败';"
        "}"
        "}).catch(e=>logContent.textContent='网络错误: '+e.message)"
        "}"
        "function hideTaskLog(){"
        "document.getElementById('task-log').style.display='none'"
        "}"
        "</script>"
        "</body></html>";
    
    httpd_resp_set_type(req, "text/html");
    httpd_resp_set_hdr(req, "Content-Encoding", "identity");
    return httpd_resp_send(req, html, strlen(html));
}

// AI任务控制API处理函数
static esp_err_t ai_task_handler(httpd_req_t *req)
{
    ESP_LOGI(TAG, "收到AI任务控制API请求");
    
    char content[256];
    size_t recv_size = MIN(req->content_len, sizeof(content) - 1);
    
    int ret = httpd_req_recv(req, content, recv_size);
    if (ret <= 0) {
        ESP_LOGE(TAG, "接收AI任务HTTP请求数据失败");
        httpd_resp_send_500(req);
        return ESP_FAIL;
    }
    content[ret] = '\0';
    ESP_LOGI(TAG, "接收到的AI任务JSON: %s", content);
    
    cJSON *json = cJSON_Parse(content);
    if (json == NULL) {
        ESP_LOGE(TAG, "AI任务JSON解析失败");
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Invalid JSON");
        return ESP_FAIL;
    }
    
    cJSON *action_obj = cJSON_GetObjectItem(json, "action");
    if (!cJSON_IsString(action_obj)) {
        ESP_LOGE(TAG, "action字段不是字符串或不存在");
        cJSON_Delete(json);
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Missing action field");
        return ESP_FAIL;
    }
    
    const char *action = cJSON_GetStringValue(action_obj);
    esp_err_t result = ESP_FAIL;
    
    cJSON *response = cJSON_CreateObject();
    
    if (strcmp(action, "start") == 0) {
        cJSON *target_obj = cJSON_GetObjectItem(json, "target");
        cJSON *timeout_obj = cJSON_GetObjectItem(json, "timeout");
        
        if (cJSON_IsString(target_obj) && cJSON_IsNumber(timeout_obj)) {
            const char *target = cJSON_GetStringValue(target_obj);
            int timeout = (int)cJSON_GetNumberValue(timeout_obj);
            
            ESP_LOGI(TAG, "🎯 启动AI搜索任务: %s (超时: %d秒)", target, timeout);
            result = local_ai_start_task(target, timeout);
            
            if (result == ESP_OK) {
                cJSON_AddBoolToObject(response, "success", true);
                cJSON_AddStringToObject(response, "message", "AI搜索任务已启动");
                ESP_LOGI(TAG, "✅ AI搜索任务启动成功");
            } else {
                cJSON_AddBoolToObject(response, "success", false);
                cJSON_AddStringToObject(response, "error", "AI搜索任务启动失败");
                ESP_LOGI(TAG, "❌ AI搜索任务启动失败");
            }
        } else {
            cJSON_AddBoolToObject(response, "success", false);
            cJSON_AddStringToObject(response, "error", "缺少目标物品或超时时间");
        }
    } else if (strcmp(action, "stop") == 0) {
        ESP_LOGI(TAG, "⏹️ 停止AI搜索任务");
        result = local_ai_stop_task();
        
        if (result == ESP_OK) {
            cJSON_AddBoolToObject(response, "success", true);
            cJSON_AddStringToObject(response, "message", "AI搜索任务已停止");
            ESP_LOGI(TAG, "✅ AI搜索任务停止成功");
        } else {
            cJSON_AddBoolToObject(response, "success", false);
            cJSON_AddStringToObject(response, "error", "AI搜索任务停止失败");
            ESP_LOGI(TAG, "❌ AI搜索任务停止失败");
        }
    } else {
        cJSON_AddBoolToObject(response, "success", false);
        cJSON_AddStringToObject(response, "error", "未知的操作");
    }
    
    char *response_str = cJSON_Print(response);
    httpd_resp_set_type(req, "application/json");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
    httpd_resp_send(req, response_str, strlen(response_str));
    
    free(response_str);
    cJSON_Delete(response);
    cJSON_Delete(json);
    
    return ESP_OK;
}

// AI任务状态查询API处理函数
static esp_err_t ai_task_status_handler(httpd_req_t *req)
{
    const ai_task_t* task_status = ai_service_get_search_status();
    
    cJSON *response = cJSON_CreateObject();
    
    // 转换状态枚举为字符串
    const char *status_str;
    switch (task_status->status) {
        case AI_TASK_IDLE:
            status_str = "idle";
            break;
        case AI_TASK_SEARCHING:
            status_str = "searching";
            break;
        case AI_TASK_COMPLETED:
            status_str = "completed";
            break;
        case AI_TASK_FAILED_TIMEOUT:
            status_str = "failed_timeout";
            break;
        case AI_TASK_FAILED_UNABLE:
            status_str = "failed_unable";
            break;
        default:
            status_str = "unknown";
            break;
    }
    
    cJSON_AddStringToObject(response, "status", status_str);
    cJSON_AddStringToObject(response, "message", task_status->status_message);
    cJSON_AddStringToObject(response, "target", task_status->target_object);
    cJSON_AddNumberToObject(response, "timeout", task_status->timeout_seconds);
    cJSON_AddNumberToObject(response, "result_count", task_status->result_count);
    
    // 添加检测结果
    if (task_status->result_count > 0) {
        cJSON *results = cJSON_CreateArray();
        for (int i = 0; i < task_status->result_count; i++) {
            cJSON *result = cJSON_CreateObject();
            cJSON_AddStringToObject(result, "class_name", task_status->results[i].class_name);
            cJSON_AddNumberToObject(result, "confidence", task_status->results[i].confidence);
            cJSON_AddNumberToObject(result, "x", task_status->results[i].x);
            cJSON_AddNumberToObject(result, "y", task_status->results[i].y);
            cJSON_AddNumberToObject(result, "width", task_status->results[i].width);
            cJSON_AddNumberToObject(result, "height", task_status->results[i].height);
            cJSON_AddItemToArray(results, result);
        }
        cJSON_AddItemToObject(response, "results", results);
    }
    
    char *response_str = cJSON_Print(response);
    httpd_resp_set_type(req, "application/json");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
    httpd_resp_send(req, response_str, strlen(response_str));
    
    free(response_str);
    cJSON_Delete(response);
    
    return ESP_OK;
}

// AI导航任务控制API处理函数
static esp_err_t ai_navigation_task_handler(httpd_req_t *req)
{
    ESP_LOGI(TAG, "收到AI导航任务控制API请求");
    
    char content[256];
    size_t recv_size = MIN(req->content_len, sizeof(content) - 1);
    
    int ret = httpd_req_recv(req, content, recv_size);
    if (ret <= 0) {
        ESP_LOGE(TAG, "接收AI导航任务HTTP请求数据失败");
        httpd_resp_send_500(req);
        return ESP_FAIL;
    }
    content[ret] = '\0';
    ESP_LOGI(TAG, "接收到的AI导航任务JSON: %s", content);
    
    cJSON *json = cJSON_Parse(content);
    if (json == NULL) {
        ESP_LOGE(TAG, "AI导航任务JSON解析失败");
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Invalid JSON");
        return ESP_FAIL;
    }
    
    cJSON *action_obj = cJSON_GetObjectItem(json, "action");
    cJSON *target_obj = cJSON_GetObjectItem(json, "target_object");
    cJSON *timeout_obj = cJSON_GetObjectItem(json, "timeout");
    cJSON *navigation_obj = cJSON_GetObjectItem(json, "use_navigation");
    
    if (!action_obj || !cJSON_IsString(action_obj)) {
        ESP_LOGE(TAG, "缺少或无效的action参数");
        cJSON_Delete(json);
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Missing or invalid action");
        return ESP_FAIL;
    }
    
    const char *action = action_obj->valuestring;
    ESP_LOGI(TAG, "AI导航任务动作: %s", action);
    
    // 设置响应头
    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
    httpd_resp_set_type(req, "application/json");
    
    cJSON *response_json = cJSON_CreateObject();
    
    if (strcmp(action, "start") == 0) {
        if (!target_obj || !cJSON_IsString(target_obj)) {
            cJSON_AddStringToObject(response_json, "status", "error");
            cJSON_AddStringToObject(response_json, "message", "缺少目标物体参数");
        } else {
            const char *target_object = target_obj->valuestring;
            int timeout_seconds = (timeout_obj && cJSON_IsNumber(timeout_obj)) ? timeout_obj->valueint : 60;
            bool use_navigation = (navigation_obj && cJSON_IsTrue(navigation_obj)) ? true : false;
            
            esp_err_t result = ai_service_start_object_search(target_object, timeout_seconds, use_navigation);
            
            if (result == ESP_OK) {
                cJSON_AddStringToObject(response_json, "status", "success");
                cJSON_AddStringToObject(response_json, "message", "AI导航任务启动成功");
                cJSON_AddStringToObject(response_json, "target_object", target_object);
                cJSON_AddNumberToObject(response_json, "timeout", timeout_seconds);
                cJSON_AddBoolToObject(response_json, "use_navigation", use_navigation);
                ESP_LOGI(TAG, "AI导航任务启动成功: %s", target_object);
            } else {
                cJSON_AddStringToObject(response_json, "status", "error");
                cJSON_AddStringToObject(response_json, "message", "AI导航任务启动失败");
                ESP_LOGE(TAG, "AI导航任务启动失败");
            }
        }
    } else if (strcmp(action, "stop") == 0) {
        esp_err_t result = ai_service_stop_object_search();
        
        if (result == ESP_OK) {
            cJSON_AddStringToObject(response_json, "status", "success");
            cJSON_AddStringToObject(response_json, "message", "AI导航任务停止成功");
            ESP_LOGI(TAG, "AI导航任务停止成功");
        } else {
            cJSON_AddStringToObject(response_json, "status", "error");
            cJSON_AddStringToObject(response_json, "message", "AI导航任务停止失败");
            ESP_LOGE(TAG, "AI导航任务停止失败");
        }
    } else {
        cJSON_AddStringToObject(response_json, "status", "error");
        cJSON_AddStringToObject(response_json, "message", "未知的动作");
        ESP_LOGE(TAG, "未知的AI导航任务动作: %s", action);
    }
    
    char *response = cJSON_Print(response_json);
    httpd_resp_send(req, response, strlen(response));
    
    free(response);
    cJSON_Delete(response_json);
    cJSON_Delete(json);
    
    return ESP_OK;
}

// AI任务日志API处理函数
static esp_err_t ai_task_log_handler(httpd_req_t *req)
{
    ESP_LOGI(TAG, "收到AI任务日志API请求");
    
    // 设置响应头
    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
    httpd_resp_set_type(req, "application/json");
    
    cJSON *response_json = cJSON_CreateObject();
    
    // 获取任务日志
    const char *task_log = local_ai_get_task_log();
    int progress = local_ai_get_task_progress();
    bool timeout = local_ai_is_task_timeout();
    
    cJSON_AddStringToObject(response_json, "status", "success");
    cJSON_AddStringToObject(response_json, "log", task_log);
    cJSON_AddNumberToObject(response_json, "progress", progress);
    cJSON_AddBoolToObject(response_json, "timeout", timeout);
    
    char *response = cJSON_Print(response_json);
    httpd_resp_send(req, response, strlen(response));
    
    free(response);
    cJSON_Delete(response_json);
    
    return ESP_OK;
}

// --- Public Functions ---

esp_err_t web_server_start(void)
{
    httpd_handle_t server = NULL;
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.server_port = 80;
    config.max_uri_handlers = 14;
    config.stack_size = 16384;  // Increased from 4096 to 16KB for AI processing

    ESP_LOGI(TAG, "Starting server on port: '%d'", config.server_port);
    if (httpd_start(&server, &config) == ESP_OK) {
        ESP_LOGI(TAG, "Registering URI handlers");
        
        httpd_uri_t index_uri = { .uri = "/", .method = HTTP_GET, .handler = index_handler };
        httpd_register_uri_handler(server, &index_uri);

        httpd_uri_t history_api_uri = { .uri = "/api/history", .method = HTTP_GET, .handler = history_api_handler };
        httpd_register_uri_handler(server, &history_api_uri);

        httpd_uri_t time_api_uri = { .uri = "/api/time", .method = HTTP_GET, .handler = time_api_handler };
        httpd_register_uri_handler(server, &time_api_uri);

        httpd_uri_t network_api_uri = { .uri = "/api/network", .method = HTTP_GET, .handler = network_api_handler };
        httpd_register_uri_handler(server, &network_api_uri);

        httpd_uri_t image_uri = { .uri = "/image", .method = HTTP_GET, .handler = image_handler };
        httpd_register_uri_handler(server, &image_uri);

        httpd_uri_t motor_api_uri = { .uri = "/api/motor", .method = HTTP_POST, .handler = motor_control_handler };
        httpd_register_uri_handler(server, &motor_api_uri);

        httpd_uri_t auto_drive_get_uri = { .uri = "/api/auto-drive", .method = HTTP_GET, .handler = auto_drive_api_handler };
        httpd_register_uri_handler(server, &auto_drive_get_uri);
        
        httpd_uri_t auto_drive_post_uri = { .uri = "/api/auto-drive", .method = HTTP_POST, .handler = auto_drive_api_handler };
        httpd_register_uri_handler(server, &auto_drive_post_uri);



        httpd_uri_t ai_task_uri = { .uri = "/api/ai-task", .method = HTTP_POST, .handler = ai_task_handler };
        httpd_register_uri_handler(server, &ai_task_uri);

        httpd_uri_t ai_task_status_uri = { .uri = "/api/ai-task-status", .method = HTTP_GET, .handler = ai_task_status_handler };
        httpd_register_uri_handler(server, &ai_task_status_uri);

        httpd_uri_t ai_navigation_task_uri = { .uri = "/api/ai-navigation-task", .method = HTTP_POST, .handler = ai_navigation_task_handler };
        httpd_register_uri_handler(server, &ai_navigation_task_uri);

        httpd_uri_t ai_task_log_uri = { .uri = "/api/ai-task-log", .method = HTTP_GET, .handler = ai_task_log_handler };
        httpd_register_uri_handler(server, &ai_task_log_uri);

        return ESP_OK;
    }

    ESP_LOGE(TAG, "Error starting server!");
    return ESP_FAIL;
}

bool web_server_get_auto_drive_status(void)
{
    return ai_auto_drive_enabled;
}