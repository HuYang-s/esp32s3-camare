#include <stdio.h>
#include <string.h>
#include <time.h>
#include <sys/unistd.h>
#include <sys/stat.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_http_server.h"
#include "esp_http_client.h"
#include "esp_spiffs.h"
#include "mbedtls/base64.h"
#include "cJSON.h"
#include "esp_crt_bundle.h"
#include "camera_driver.h"

static const char *TAG = "camera_web";

#define WIFI_SSID "bed_room_2.4G"
#define WIFI_PASS "Hdk4560.0"

// AI API配置
#define AI_API_KEY "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"
#define AI_BASE_URL "https://integrate.api.nvidia.com"
#define AI_MODEL "meta/llama-4-maverick-17b-128e-instruct"
#define MAX_HTTP_RECV_BUFFER 8192
#define MAX_HTTP_OUTPUT_BUFFER 8192

typedef struct {
    char filename[64];
    time_t capture_time;
} image_info_t;

static SemaphoreHandle_t wifi_connected_semaphore;
static SemaphoreHandle_t analysis_complete_semaphore;

// Base64编码函数
static char* encode_image_to_base64(camera_fb_t *fb)
{
    size_t out_len = 0;
    mbedtls_base64_encode(NULL, 0, &out_len, fb->buf, fb->len);
    
    char* base64_buffer = malloc(out_len + 1);
    if (!base64_buffer) {
        ESP_LOGE(TAG, "Failed to allocate memory for base64 encoding");
        return NULL;
    }
    
    int ret = mbedtls_base64_encode((unsigned char*)base64_buffer, out_len, &out_len, fb->buf, fb->len);
    if (ret != 0) {
        ESP_LOGE(TAG, "Base64 encoding failed");
        free(base64_buffer);
        return NULL;
    }
    
    base64_buffer[out_len] = '\0';
    return base64_buffer;
}

// HTTP响应处理函数
esp_err_t _http_event_handler(esp_http_client_event_t *evt)
{
    static int output_len;
    
    switch(evt->event_id) {
        case HTTP_EVENT_ERROR:
            ESP_LOGD(TAG, "HTTP_EVENT_ERROR");
            break;
        case HTTP_EVENT_ON_CONNECTED:
            ESP_LOGD(TAG, "HTTP_EVENT_ON_CONNECTED");
            break;
        case HTTP_EVENT_HEADER_SENT:
            ESP_LOGD(TAG, "HTTP_EVENT_HEADER_SENT");
            break;
        case HTTP_EVENT_ON_HEADER:
            ESP_LOGD(TAG, "HTTP_EVENT_ON_HEADER, key=%s, value=%s", evt->header_key, evt->header_value);
            break;
        case HTTP_EVENT_ON_DATA:
            ESP_LOGD(TAG, "HTTP_EVENT_ON_DATA, len=%d", evt->data_len);
            if (!esp_http_client_is_chunked_response(evt->client)) {
                if (evt->user_data) {
                    memcpy(evt->user_data + output_len, evt->data, evt->data_len);
                    output_len += evt->data_len;
                }
            }
            break;
        case HTTP_EVENT_ON_FINISH:
            ESP_LOGD(TAG, "HTTP_EVENT_ON_FINISH");
            output_len = 0;
            break;
        case HTTP_EVENT_DISCONNECTED:
            ESP_LOGI(TAG, "HTTP_EVENT_DISCONNECTED");
            output_len = 0;
            break;
        case HTTP_EVENT_REDIRECT:
            ESP_LOGD(TAG, "HTTP_EVENT_REDIRECT");
            break;
    }
    return ESP_OK;
}

// AI分析函数
static esp_err_t analyze_image_with_ai(camera_fb_t *fb)
{
    ESP_LOGI(TAG, "开始AI分析图片...");
    
    char *base64_image = encode_image_to_base64(fb);
    if (!base64_image) {
        return ESP_FAIL;
    }
    
    // 配置HTTP客户端
    char *response_buffer = malloc(MAX_HTTP_OUTPUT_BUFFER);
    if (!response_buffer) {
        ESP_LOGE(TAG, "Failed to allocate response buffer");
        free(base64_image);
        return ESP_FAIL;
    }
    memset(response_buffer, 0, MAX_HTTP_OUTPUT_BUFFER);
    
    esp_http_client_config_t config = {
        .url = AI_BASE_URL "/v1/chat/completions",
        .method = HTTP_METHOD_POST,
        .event_handler = _http_event_handler,
        .user_data = response_buffer,
        .timeout_ms = 30000,
        .is_async = false,
        .use_global_ca_store = true,
        .crt_bundle_attach = esp_crt_bundle_attach,
    };
    
    esp_http_client_handle_t client = esp_http_client_init(&config);
    if (!client) {
        ESP_LOGE(TAG, "Failed to init HTTP client");
        free(base64_image);
        free(response_buffer);
        return ESP_FAIL;
    }
    
    // 创建JSON请求体
    cJSON *json = cJSON_CreateObject();
    cJSON *model = cJSON_CreateString(AI_MODEL);
    cJSON_AddItemToObject(json, "model", model);
    
    cJSON *messages = cJSON_CreateArray();
    cJSON *message = cJSON_CreateObject();
    cJSON *role = cJSON_CreateString("user");
    cJSON_AddItemToObject(message, "role", role);
    
    cJSON *content = cJSON_CreateArray();
    
    // 添加文本内容
    cJSON *text_content = cJSON_CreateObject();
    cJSON *text_type = cJSON_CreateString("text");
    cJSON *text_text = cJSON_CreateString("用中文描述照片，直接用中文说照片内容，不用分析，直接说几个什么东西");
    cJSON_AddItemToObject(text_content, "type", text_type);
    cJSON_AddItemToObject(text_content, "text", text_text);
    cJSON_AddItemToArray(content, text_content);
    
    // 添加图片内容
    cJSON *image_content = cJSON_CreateObject();
    cJSON *image_type = cJSON_CreateString("image_url");
    cJSON *image_url = cJSON_CreateObject();
    
    char *image_url_str = malloc(strlen("data:image/jpeg;base64,") + strlen(base64_image) + 1);
    if (!image_url_str) {
        ESP_LOGE(TAG, "Failed to allocate image URL string");
        esp_http_client_cleanup(client);
        cJSON_Delete(json);
        free(base64_image);
        free(response_buffer);
        return ESP_FAIL;
    }
    sprintf(image_url_str, "data:image/jpeg;base64,%s", base64_image);
    cJSON *url = cJSON_CreateString(image_url_str);
    cJSON_AddItemToObject(image_url, "url", url);
    cJSON_AddItemToObject(image_content, "image_url", image_url);
    cJSON_AddItemToObject(image_content, "type", image_type);
    cJSON_AddItemToArray(content, image_content);
    
    cJSON_AddItemToObject(message, "content", content);
    cJSON_AddItemToArray(messages, message);
    cJSON_AddItemToObject(json, "messages", messages);
    
    cJSON *max_tokens = cJSON_CreateNumber(64);
    cJSON_AddItemToObject(json, "max_tokens", max_tokens);
    
    cJSON *stream = cJSON_CreateBool(false);
    cJSON_AddItemToObject(json, "stream", stream);
    
    char *json_string = cJSON_Print(json);
    if (!json_string) {
        ESP_LOGE(TAG, "Failed to create JSON string");
        esp_http_client_cleanup(client);
        cJSON_Delete(json);
        free(base64_image);
        free(response_buffer);
        free(image_url_str);
        return ESP_FAIL;
    }
    
    // 设置请求头
    esp_http_client_set_header(client, "Content-Type", "application/json");
    char auth_header[256];
    snprintf(auth_header, sizeof(auth_header), "Bearer %s", AI_API_KEY);
    esp_http_client_set_header(client, "Authorization", auth_header);
    
    esp_http_client_set_post_field(client, json_string, strlen(json_string));
    
    // 发送请求
    esp_err_t err = esp_http_client_perform(client);
    
    if (err == ESP_OK) {
        int status_code = esp_http_client_get_status_code(client);
        ESP_LOGI(TAG, "HTTP POST Status = %d", status_code);
        
        if (status_code == 200) {
            ESP_LOGI(TAG, "AI分析响应: %s", response_buffer);
            
            // 解析响应JSON
            cJSON *response_json = cJSON_Parse(response_buffer);
            if (response_json) {
                cJSON *choices = cJSON_GetObjectItem(response_json, "choices");
                if (choices && cJSON_GetArraySize(choices) > 0) {
                    cJSON *choice = cJSON_GetArrayItem(choices, 0);
                    cJSON *message_obj = cJSON_GetObjectItem(choice, "message");
                    cJSON *content_obj = cJSON_GetObjectItem(message_obj, "content");
                    
                    if (content_obj && cJSON_IsString(content_obj)) {
                        printf("\n=== AI分析结果 ===\n");
                        printf("%s\n", content_obj->valuestring);
                        printf("==================\n\n");
                    }
                }
                cJSON_Delete(response_json);
            }
        } else {
            ESP_LOGE(TAG, "HTTP请求失败，状态码: %d", status_code);
            ESP_LOGE(TAG, "响应内容: %s", response_buffer);
        }
    } else {
        ESP_LOGE(TAG, "HTTP请求错误: %s", esp_err_to_name(err));
    }
    
    // 清理资源
    esp_http_client_cleanup(client);
    cJSON_Delete(json);
    free(json_string);
    free(base64_image);
    free(response_buffer);
    free(image_url_str);
    
    return err;
}

static esp_err_t jpg_handler(httpd_req_t *req)
{
    camera_fb_t *fb = NULL;
    esp_err_t res = ESP_OK;

    res = camera_capture(&fb);
    if (res != ESP_OK) {
        httpd_resp_send_500(req);
        return ESP_FAIL;
    }

    httpd_resp_set_type(req, "image/jpeg");
    httpd_resp_set_hdr(req, "Content-Disposition", "inline; filename=capture.jpg");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");

    res = httpd_resp_send(req, (const char *)fb->buf, fb->len);
    camera_return_fb(fb);
    return res;
}

static esp_err_t index_handler(httpd_req_t *req)
{
    const char* html = 
        "<!DOCTYPE html>\n"
        "<html>\n"
        "<head>\n"
        "    <title>ESP32-S3 Camera</title>\n"
        "    <meta charset=\"utf-8\">\n"
        "    <style>\n"
        "        body { text-align: center; font-family: Arial, sans-serif; }\n"
        "        img { max-width: 100%; height: auto; border: 2px solid #ddd; }\n"
        "        button { padding: 10px 20px; font-size: 16px; margin: 10px; }\n"
        "    </style>\n"
        "</head>\n"
        "<body>\n"
        "    <h1>ESP32-S3 OV3640 Camera</h1>\n"
        "    <img id=\"stream\" src=\"/capture\" width=\"800\">\n"
        "    <br>\n"
        "    <button onclick=\"location.reload()\">Refresh</button>\n"
        "    <button onclick=\"startStream()\">Start Stream</button>\n"
        "    <button onclick=\"stopStream()\">Stop Stream</button>\n"
        "    <script>\n"
        "        let streaming = false;\n"
        "        let streamInterval;\n"
        "        function startStream() {\n"
        "            if (!streaming) {\n"
        "                streaming = true;\n"
        "                streamInterval = setInterval(() => {\n"
        "                    document.getElementById('stream').src = '/capture?' + new Date().getTime();\n"
        "                }, 500);\n"
        "            }\n"
        "        }\n"
        "        function stopStream() {\n"
        "            if (streaming) {\n"
        "                streaming = false;\n"
        "                clearInterval(streamInterval);\n"
        "            }\n"
        "        }\n"
        "    </script>\n"
        "</body>\n"
        "</html>";

    httpd_resp_set_type(req, "text/html");
    return httpd_resp_send(req, html, strlen(html));
}

static httpd_handle_t start_webserver(void)
{
    httpd_handle_t server = NULL;
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.server_port = 80;

    ESP_LOGI(TAG, "Starting server on port: '%d'", config.server_port);
    if (httpd_start(&server, &config) == ESP_OK) {
        ESP_LOGI(TAG, "Registering URI handlers");
        
        httpd_uri_t index_uri = {
            .uri       = "/",
            .method    = HTTP_GET,
            .handler   = index_handler,
            .user_ctx  = NULL
        };
        httpd_register_uri_handler(server, &index_uri);

        httpd_uri_t capture_uri = {
            .uri       = "/capture",
            .method    = HTTP_GET,
            .handler   = jpg_handler,
            .user_ctx  = NULL
        };
        httpd_register_uri_handler(server, &capture_uri);

        return server;
    }

    ESP_LOGI(TAG, "Error starting server!");
    return NULL;
}

static void wifi_event_handler(void* arg, esp_event_base_t event_base,
                              int32_t event_id, void* event_data)
{
    (void)arg;
    (void)event_data;
    
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        ESP_LOGI(TAG, "WiFi disconnected, retrying...");
        esp_wifi_connect();
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI(TAG, "Got IP:" IPSTR, IP2STR(&event->ip_info.ip));
        xSemaphoreGive(wifi_connected_semaphore);
    }
}

static void wifi_init_sta(void)
{
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();

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

    wifi_config_t wifi_config = {
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

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "wifi_init_sta finished. Connecting to SSID:%s", WIFI_SSID);
}

static esp_err_t init_spiffs(void)
{
    esp_vfs_spiffs_conf_t conf = {
        .base_path = "/spiffs",
        .partition_label = NULL,
        .max_files = 5,
        .format_if_mount_failed = true
    };

    esp_err_t ret = esp_vfs_spiffs_register(&conf);
    if (ret != ESP_OK) {
        if (ret == ESP_FAIL) {
            ESP_LOGE(TAG, "Failed to mount or format filesystem");
        } else if (ret == ESP_ERR_NOT_FOUND) {
            ESP_LOGE(TAG, "Failed to find SPIFFS partition");
        } else {
            ESP_LOGE(TAG, "Failed to initialize SPIFFS (%s)", esp_err_to_name(ret));
        }
        return ret;
    }

    size_t total = 0, used = 0;
    ret = esp_spiffs_info(NULL, &total, &used);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to get SPIFFS partition information (%s)", esp_err_to_name(ret));
    } else {
        ESP_LOGI(TAG, "Partition size: total: %d, used: %d", total, used);
    }

    return ESP_OK;
}

static esp_err_t save_image_to_file(camera_fb_t *fb)
{
    char filename[64];
    time_t now;
    time(&now);
    
    snprintf(filename, sizeof(filename), "/spiffs/img_%lld.jpg", (long long)now);
    
    FILE *f = fopen(filename, "w");
    if (f == NULL) {
        ESP_LOGE(TAG, "Failed to open file for writing");
        return ESP_FAIL;
    }
    
    size_t written = fwrite(fb->buf, 1, fb->len, f);
    fclose(f);
    
    if (written != fb->len) {
        ESP_LOGE(TAG, "Failed to write complete image");
        return ESP_FAIL;
    }
    
    ESP_LOGI(TAG, "Image saved to %s (%d bytes)", filename, fb->len);
    return ESP_OK;
}

static void capture_and_analyze_task(void *pvParameters)
{
    camera_fb_t *fb = NULL;
    
    while (1) {
        printf("\n=== 开始拍照 ===\n");
        ESP_LOGI(TAG, "正在拍照...");
        
        esp_err_t res = camera_capture(&fb);
        if (res == ESP_OK && fb != NULL) {
            printf("拍照完成，图片大小: %d 字节\n", fb->len);
            ESP_LOGI(TAG, "拍照成功，开始AI分析");
            
            // 可选：保存图片到SPIFFS
            save_image_to_file(fb);
            
            // AI分析
            esp_err_t ai_result = analyze_image_with_ai(fb);
            if (ai_result == ESP_OK) {
                printf("AI分析完成\n");
                ESP_LOGI(TAG, "AI分析完成");
            } else {
                printf("AI分析失败\n");
                ESP_LOGE(TAG, "AI分析失败");
            }
            
            camera_return_fb(fb);
            printf("等待下次拍照...\n\n");
        } else {
            ESP_LOGE(TAG, "Camera capture failed");
            printf("拍照失败，重试中...\n");
        }
        
        // 等待10秒后拍下一张
        vTaskDelay(pdMS_TO_TICKS(10000));
    }
}

void app_main(void)
{
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    // 创建信号量
    wifi_connected_semaphore = xSemaphoreCreateBinary();
    analysis_complete_semaphore = xSemaphoreCreateBinary();
    
    ESP_ERROR_CHECK(init_spiffs());

    ESP_LOGI(TAG, "连接WiFi网络...");
    wifi_init_sta();
    
    // 等待WiFi连接
    if (xSemaphoreTake(wifi_connected_semaphore, pdMS_TO_TICKS(30000)) == pdTRUE) {
        ESP_LOGI(TAG, "WiFi连接成功");
        printf("WiFi连接成功，开始初始化摄像头...\n");
    } else {
        ESP_LOGE(TAG, "WiFi连接超时");
        printf("WiFi连接失败，请检查网络配置\n");
        return;
    }

    ESP_ERROR_CHECK(camera_init());
    printf("摄像头初始化完成\n");

    // 启动web服务器（可选）
    start_webserver();

    // 创建拍照和AI分析任务
    xTaskCreate(capture_and_analyze_task, "capture_analyze_task", 16384, NULL, 5, NULL);

    printf("\n系统启动完成，开始自动拍照和AI分析\n");
    printf("连接信息：WiFi '%s'\n", WIFI_SSID);
    printf("Web界面：http://[设备IP地址]\n\n");
    
    ESP_LOGI(TAG, "系统启动完成，每10秒自动拍照并进行AI分析");
}
