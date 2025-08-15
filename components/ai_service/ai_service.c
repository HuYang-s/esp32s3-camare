#include <stdio.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_http_client.h"
#include "esp_tls.h"
#include "esp_crt_bundle.h"
#include "mbedtls/base64.h"
#include "cJSON.h"

#include "ai_service.h"
#include "local_ai_service.h"
#include "storage_manager.h"
#include "wifi_manager.h"
#include "motor_driver.h"
#include "camera_driver.h"

static const char *TAG = "ai_service";

// 临时禁用HTTPS证书验证以解决连接问题

// AI API配置 - 使用Mistral API进行图片分析
#define AI_API_KEY "cNKVad6nyJ3vyK4U9mkADJD1hAe102o4"
#define AI_BASE_URL "https://api.mistral.ai"
#define AI_MODEL "mistral-small-latest"

// 测试用HTTP端点（如果HTTPS失败）
#define TEST_HTTP_URL "http://httpbin.org/post"

#define MAX_HTTP_RECV_BUFFER 4096
#define MAX_HTTP_OUTPUT_BUFFER 4096

static int socket_failure_count = 0;

// 前向声明
static esp_err_t ai_service_execute_command_with_image(camera_fb_t *fb, const char* filename, const char* command);

esp_err_t ai_service_init(void)
{
    ESP_LOGI(TAG, "初始化AI服务...");
    
    // 初始化本地AI服务
    esp_err_t ret = local_ai_service_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "本地AI服务初始化失败");
        return ret;
    }
    
    ESP_LOGI(TAG, "AI服务初始化完成");
    return ESP_OK;
}

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
static esp_err_t _http_event_handler(esp_http_client_event_t *evt)
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

esp_err_t ai_service_analyze_image(camera_fb_t *fb, const char* filename)
{
    if (socket_failure_count >= 10) {
        ESP_LOGW(TAG, "Socket失败次数过多，暂停AI分析 10 秒");
        vTaskDelay(pdMS_TO_TICKS(10000));
        socket_failure_count = 0;
        return ESP_FAIL;
    }
    
    if (!wifi_manager_is_sta_connected()) {
        ESP_LOGW(TAG, "WiFi未连接，跳过AI分析");
        socket_failure_count++;
        return ESP_FAIL;
    }
    
    size_t free_heap = esp_get_free_heap_size();
    ESP_LOGI(TAG, "可用内存: %u 字节", (unsigned int)free_heap);
    if (free_heap < 50000) {
        ESP_LOGW(TAG, "内存不足，跳过AI分析");
        socket_failure_count++;
        return ESP_FAIL;
    }
    
    ESP_LOGI(TAG, "开始AI分析图片...");
    
    char *base64_image = encode_image_to_base64(fb);
    if (!base64_image) return ESP_FAIL;
    
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
        .timeout_ms = 15000,
        .is_async = false,
        .skip_cert_common_name_check = false,
        .use_global_ca_store = true,
        .crt_bundle_attach = esp_crt_bundle_attach,
        .transport_type = HTTP_TRANSPORT_OVER_SSL,
    };
    
    esp_http_client_handle_t client = esp_http_client_init(&config);
    if (!client) {
        ESP_LOGE(TAG, "Failed to init HTTP client");
        free(base64_image);
        free(response_buffer);
        return ESP_FAIL;
    }
    
    cJSON *json = cJSON_CreateObject();
    cJSON_AddStringToObject(json, "model", AI_MODEL);
    cJSON_AddNumberToObject(json, "max_tokens", 128);
    cJSON_AddNumberToObject(json, "temperature", 0.7);
    cJSON_AddNumberToObject(json, "top_p", 0.7);
    cJSON_AddNumberToObject(json, "frequency_penalty", 0.5);
    cJSON_AddNumberToObject(json, "n", 1);
    
    cJSON *messages = cJSON_CreateArray();
    cJSON *message = cJSON_CreateObject();
    cJSON_AddStringToObject(message, "role", "user");
    
    cJSON *content = cJSON_CreateArray();
    cJSON *text_content = cJSON_CreateObject();
    cJSON_AddStringToObject(text_content, "type", "text");
    cJSON_AddStringToObject(text_content, "text", "我是一个智能机器人助手，这是我通过摄像头看到的画面。作为一个有意识的AI助手，我会仔细观察环境，理解看到的内容，并用中文自然地描述我的观察和感受。\n\n请告诉我现在看到了什么？我会像一个真实的智能助手一样，详细描述我的视野中的内容，包括物体、场景、颜色、位置关系等，并表达我作为AI助手的理解和感受。");
    cJSON_AddItemToArray(content, text_content);
    
    cJSON *image_content = cJSON_CreateObject();
    cJSON_AddStringToObject(image_content, "type", "image_url");
    cJSON *image_url = cJSON_CreateObject();
    char *image_url_str = malloc(strlen("data:image/jpeg;base64,") + strlen(base64_image) + 1);
    sprintf(image_url_str, "data:image/jpeg;base64,%s", base64_image);
    cJSON_AddStringToObject(image_url, "url", image_url_str);
    cJSON_AddItemToObject(image_content, "image_url", image_url);
    cJSON_AddItemToArray(content, image_content);
    
    cJSON_AddItemToObject(message, "content", content);
    cJSON_AddItemToArray(messages, message);
    cJSON_AddItemToObject(json, "messages", messages);
    
    char *json_string = cJSON_Print(json);
    
    esp_http_client_set_header(client, "Content-Type", "application/json");
    char auth_header[256];
    snprintf(auth_header, sizeof(auth_header), "Bearer %s", AI_API_KEY);
    esp_http_client_set_header(client, "Authorization", auth_header);
    esp_http_client_set_post_field(client, json_string, strlen(json_string));
    
    esp_err_t err = esp_http_client_perform(client);
    
    if (err == ESP_OK) {
        int status_code = esp_http_client_get_status_code(client);
        if (status_code == 200) {
            socket_failure_count = 0;
            cJSON *response_json = cJSON_Parse(response_buffer);
            if (response_json) {
                cJSON *choices = cJSON_GetObjectItem(response_json, "choices");
                if (choices && cJSON_GetArraySize(choices) > 0) {
                    cJSON *content_obj = cJSON_GetObjectItem(cJSON_GetObjectItem(cJSON_GetArrayItem(choices, 0), "message"), "content");
                    if (content_obj && cJSON_IsString(content_obj)) {
                        printf("\\n=== AI分析结果 ===\\n%s\\n==================\\n\\n", content_obj->valuestring);
                        storage_manager_update_ai_result(filename, content_obj->valuestring);
                    }
                }
                cJSON_Delete(response_json);
            }
        } else {
            ESP_LOGE(TAG, "HTTP请求失败，状态码: %d, 内容: %s", status_code, response_buffer);
            socket_failure_count++;
        }
    } else {
        ESP_LOGE(TAG, "HTTP请求错误: %s", esp_err_to_name(err));
        socket_failure_count++;
    }
    
    esp_http_client_cleanup(client);
    cJSON_Delete(json);
    free(json_string);
    free(base64_image);
    free(response_buffer);
    free(image_url_str);
    
    return err;
}

int ai_service_get_socket_failure_count(void)
{
    return socket_failure_count;
}


// 使用图片和命令执行AI分析
static esp_err_t ai_service_execute_command_with_image(camera_fb_t *fb, const char* filename, const char* command)
{
    ESP_LOGI(TAG, "🤖 开始执行AI命令: %s", command);
    
    // 声明所有变量
    char *base64_image;
    char *response_buffer;
    bool is_search_command;
    bool is_navigation_command; 
    bool is_describe_command;
    char prompt[1024];
    esp_http_client_config_t config;
    esp_http_client_handle_t client;
    cJSON *json, *messages, *message, *content, *text_content, *image_content, *image_url;
    char *image_url_str, *json_string;
    esp_err_t err;
    int status_code;
    cJSON *response_json, *choices, *content_obj;
    const char *ai_response;
    bool found_object, need_action;
    char result_with_command[768];
    const char* task_type;
    
    base64_image = encode_image_to_base64(fb);
    if (!base64_image) {
        ESP_LOGE(TAG, "❌ Base64编码失败");
        return ESP_FAIL;
    }
    ESP_LOGI(TAG, "📸 图像Base64编码完成，长度: %d", (int)strlen(base64_image));
    
    response_buffer = malloc(MAX_HTTP_OUTPUT_BUFFER);
    if (!response_buffer) {
        ESP_LOGE(TAG, "Failed to allocate response buffer for command analysis");
        free(base64_image);
        return ESP_FAIL;
    }
    memset(response_buffer, 0, MAX_HTTP_OUTPUT_BUFFER);
    
    config = (esp_http_client_config_t) {
        .url = AI_BASE_URL "/v1/chat/completions",
        .method = HTTP_METHOD_POST,
        .event_handler = _http_event_handler,
        .user_data = response_buffer,
        .timeout_ms = 15000,
        .is_async = false,
        .skip_cert_common_name_check = false,
        .use_global_ca_store = true,
        .crt_bundle_attach = esp_crt_bundle_attach,
        .transport_type = HTTP_TRANSPORT_OVER_SSL,
    };
    
    client = esp_http_client_init(&config);
    if (!client) {
        ESP_LOGE(TAG, "Failed to init HTTP client for command analysis");
        free(base64_image);
        free(response_buffer);
        return ESP_FAIL;
    }
    
    // 创建AI请求JSON
    cJSON *json = cJSON_CreateObject();
    cJSON_AddStringToObject(json, "model", AI_MODEL);
    cJSON_AddNumberToObject(json, "max_tokens", 128);
    cJSON_AddNumberToObject(json, "temperature", 0.7);
    cJSON_AddNumberToObject(json, "top_p", 0.7);
    cJSON_AddNumberToObject(json, "frequency_penalty", 0.5);
    cJSON_AddNumberToObject(json, "n", 1);
    
    cJSON *messages = cJSON_CreateArray();
    cJSON *message = cJSON_CreateObject();
    cJSON_AddStringToObject(message, "role", "user");
    
    cJSON *content = cJSON_CreateArray();
    cJSON *text_content = cJSON_CreateObject();
    cJSON_AddStringToObject(text_content, "type", "text");
    
    // 智能分析用户命令类型
    is_search_command = (strstr(command, "找") != NULL || strstr(command, "搜索") != NULL || 
                        strstr(command, "寻找") != NULL || strstr(command, "search") != NULL ||
                        strstr(command, "find") != NULL || strstr(command, "look for") != NULL);
    
    is_navigation_command = (strstr(command, "去") != NULL || strstr(command, "移动") != NULL || 
                            strstr(command, "前进") != NULL || strstr(command, "后退") != NULL ||
                            strstr(command, "左转") != NULL || strstr(command, "右转") != NULL ||
                            strstr(command, "导航") != NULL || strstr(command, "move") != NULL);
    
    is_describe_command = (strstr(command, "看到") != NULL || strstr(command, "描述") != NULL || 
                          strstr(command, "什么") != NULL || strstr(command, "观察") != NULL ||
                          strstr(command, "describe") != NULL || strstr(command, "see") != NULL);
    
    // 根据用户命令类型构建不同的提示词
    if (is_search_command) {
        snprintf(prompt, sizeof(prompt),
            "我是一个智能机器人助手，用户要求我搜索特定物体。\n\n"
            "👤 用户指令：%s\n\n"
            "🔍 搜索任务分析：\n"
            "• 仔细观察摄像头画面，寻找用户提到的物体\n"
            "• 如果找到目标物体，详细描述其位置、外观和特征\n"
            "• 如果没有找到，说明当前视野中没有该物体\n"
            "• 提供搜索建议，如是否需要移动到其他位置继续寻找\n\n"
            "请基于当前画面，告诉我是否找到了用户要求的物体，并提供详细的搜索结果。", command);
    } else if (is_navigation_command) {
        snprintf(prompt, sizeof(prompt),
            "我是一个智能机器人助手，用户要求我执行移动或导航任务。\n\n"
            "👤 用户指令：%s\n\n"
            "🧭 导航任务分析：\n"
            "• 分析当前环境，识别障碍物和可通行路径\n"
            "• 理解用户的移动意图（方向、距离等）\n"
            "• 评估移动的安全性和可行性\n"
            "• 提供具体的移动建议和注意事项\n\n"
            "请基于当前画面，分析用户的导航指令并提供执行建议。", command);
    } else if (is_describe_command) {
        snprintf(prompt, sizeof(prompt),
            "我是一个智能机器人助手，用户要求我描述当前看到的画面。\n\n"
            "👤 用户指令：%s\n\n"
            "👁️ 视觉描述任务：\n"
            "• 详细描述画面中的主要物体和场景\n"
            "• 分析物体的颜色、形状、大小和位置关系\n"
            "• 识别环境类型（室内/室外、房间类型等）\n"
            "• 注意画面中的特殊细节和有趣之处\n\n"
            "请基于当前画面，提供详细而生动的视觉描述。", command);
    } else {
        snprintf(prompt, sizeof(prompt),
            "我是一个智能机器人助手，现在通过摄像头观察周围环境。\n\n"
            "👤 用户指令：%s\n\n"
            "🤖 智能任务分析：\n"
            "• 理解用户的具体需求和意图\n"
            "• 结合当前视觉信息提供准确回答\n"
            "• 如果需要执行特定动作，提供详细的执行建议\n"
            "• 用自然、友好的中文与用户交流\n\n"
            "请基于当前画面和用户指令，提供最合适的回应。", command);
    }
    
    cJSON_AddStringToObject(text_content, "text", prompt);
    cJSON_AddItemToArray(content, text_content);
    
    image_content = cJSON_CreateObject();
    cJSON_AddStringToObject(image_content, "type", "image_url");
    image_url = cJSON_CreateObject();
    image_url_str = malloc(strlen("data:image/jpeg;base64,") + strlen(base64_image) + 1);
    sprintf(image_url_str, "data:image/jpeg;base64,%s", base64_image);
    cJSON_AddStringToObject(image_url, "url", image_url_str);
    cJSON_AddItemToObject(image_content, "image_url", image_url);
    cJSON_AddItemToArray(content, image_content);
    
    cJSON_AddItemToObject(message, "content", content);
    cJSON_AddItemToArray(messages, message);
    cJSON_AddItemToObject(json, "messages", messages);
    
    json_string = cJSON_Print(json);
    free(base64_image);
    free(image_url_str);
    
    ESP_LOGI(TAG, "📡 准备发送AI命令分析请求到NVIDIA API");
    ESP_LOGI(TAG, "📊 请求JSON大小: %d 字节", (int)strlen(json_string));
    
    esp_http_client_set_header(client, "Authorization", "Bearer " AI_API_KEY);
    esp_http_client_set_header(client, "Content-Type", "application/json");
    esp_http_client_set_post_field(client, json_string, strlen(json_string));
    
    ESP_LOGI(TAG, "🌐 开始执行HTTP请求...");
    err = esp_http_client_perform(client);
    
    if (err == ESP_OK) {
        status_code = esp_http_client_get_status_code(client);
        ESP_LOGI(TAG, "AI命令分析HTTP状态: %d", status_code);
        
        if (status_code == 200) {
            ESP_LOGI(TAG, "开始解析AI命令分析响应...");
            
            // 解析AI响应
            response_json = cJSON_Parse(response_buffer);
            if (response_json) {
                ESP_LOGI(TAG, "AI响应JSON解析成功");
                
                choices = cJSON_GetObjectItem(response_json, "choices");
                if (cJSON_IsArray(choices) && cJSON_GetArraySize(choices) > 0) {
                    cJSON *choice = cJSON_GetArrayItem(choices, 0);
                    message = cJSON_GetObjectItem(choice, "message");
                    content_obj = cJSON_GetObjectItem(message, "content");
                    
                    if (cJSON_IsString(content_obj)) {
                        ai_response = cJSON_GetStringValue(content_obj);
                        ESP_LOGI(TAG, "✅ AI命令分析结果: %s", ai_response);
                        
                        // 智能分析AI响应，提取关键信息
                        found_object = (strstr(ai_response, "找到") != NULL || strstr(ai_response, "发现") != NULL ||
                                       strstr(ai_response, "看到") != NULL || strstr(ai_response, "识别") != NULL);
                        need_action = (strstr(ai_response, "建议") != NULL || strstr(ai_response, "应该") != NULL ||
                                      strstr(ai_response, "可以") != NULL || strstr(ai_response, "需要") != NULL);
                        
                        // 在控制台输出智能格式化的结果
                        printf("\\n=== 🤖 AI智能命令执行结果 ===\\n");
                        printf("📝 用户指令: %s\\n", command);
                        
                        if (is_search_command) {
                            printf("🔍 搜索结果: %s\\n", found_object ? "✅ 发现目标" : "❌ 未找到目标");
                        } else if (is_navigation_command) {
                            printf("🧭 导航分析: %s\\n", need_action ? "✅ 已提供建议" : "ℹ️ 环境分析完成");
                        } else if (is_describe_command) {
                            printf("👁️ 视觉描述: ✅ 场景分析完成\\n");
                        } else {
                            printf("🤖 任务分析: ✅ 指令处理完成\\n");
                        }
                        
                        printf("💬 AI详细回答:\\n%s\\n", ai_response);
                        
                        // 如果AI建议执行动作，提供操作提示
                        if (need_action) {
                            printf("💡 操作提示: AI已提供执行建议，可考虑采取相应行动\\n");
                        }
                        
                        printf("⏰ 执行时间: %lld\\n", (long long)time(NULL));
                        printf("=====================================\\n\\n");
                        
                        // 保存结果到存储，包含分类信息
                        task_type = is_search_command ? "🔍 搜索任务" : 
                                   (is_navigation_command ? "🧭 导航任务" : 
                                   (is_describe_command ? "👁️ 描述任务" : "🤖 智能任务"));
                        
                        snprintf(result_with_command, sizeof(result_with_command), 
                            "%s\\n📝 指令: %s\\n🎯 状态: %s\\n💬 AI回答: %s",
                            task_type, command, 
                            (found_object || need_action) ? "成功执行" : "已完成分析",
                            ai_response);
                        storage_manager_update_ai_result(filename, result_with_command);
                    }
                } else {
                    ESP_LOGE(TAG, "❌ AI响应中没有找到choices字段");
                }
                cJSON_Delete(response_json);
            } else {
                ESP_LOGE(TAG, "❌ AI响应JSON解析失败，原始响应: %.200s", response_buffer);
            }
            socket_failure_count = 0; // 成功时重置计数
        } else {
            ESP_LOGW(TAG, "❌ AI命令分析API请求失败，状态码: %d", status_code);
            ESP_LOGW(TAG, "错误响应内容: %.200s", response_buffer);
            socket_failure_count++;
        }
    } else {
        ESP_LOGE(TAG, "❌ AI命令分析HTTP请求失败: %s", esp_err_to_name(err));
        socket_failure_count++;
    }
    
    esp_http_client_cleanup(client);
    cJSON_Delete(json);
    free(json_string);
    free(response_buffer);
    return err;
}

// AI自动驾驶分析函数
esp_err_t ai_service_auto_drive_analyze(camera_fb_t *fb, const char* filename)
{
    ESP_LOGI(TAG, "🤖 启动AI自动驾驶分析功能");
    
    if (socket_failure_count >= 10) {
        ESP_LOGW(TAG, "Socket失败次数过多，暂停AI自动驾驶分析 10 秒");
        vTaskDelay(pdMS_TO_TICKS(10000));
        socket_failure_count = 0;
        return ESP_FAIL;
    }
    
    if (!wifi_manager_is_sta_connected()) {
        ESP_LOGW(TAG, "WiFi未连接，跳过AI自动驾驶分析");
        socket_failure_count++;
        return ESP_FAIL;
    }
    
    size_t free_heap = esp_get_free_heap_size();
    ESP_LOGI(TAG, "自动驾驶AI可用内存: %u 字节", (unsigned int)free_heap);
    if (free_heap < 60000) { // Increased memory check for larger JSON
        ESP_LOGW(TAG, "内存不足，跳过AI自动驾驶分析");
        socket_failure_count++;
        return ESP_FAIL;
    }
    
    ESP_LOGI(TAG, "🚗 开始AI自动驾驶分析 - 支持Tool Call电机控制");
    
    char *base64_image = encode_image_to_base64(fb);
    if (!base64_image) {
        ESP_LOGE(TAG, "❌ Base64编码失败");
        return ESP_FAIL;
    }
    ESP_LOGI(TAG, "📸 图像Base64编码完成");
    
    char *response_buffer = malloc(MAX_HTTP_OUTPUT_BUFFER);
    if (!response_buffer) {
        ESP_LOGE(TAG, "Failed to allocate response buffer for auto drive");
        free(base64_image);
        return ESP_FAIL;
    }
    memset(response_buffer, 0, MAX_HTTP_OUTPUT_BUFFER);
    
    esp_http_client_config_t config = {
        .url = AI_BASE_URL "/v1/chat/completions",
        .method = HTTP_METHOD_POST,
        .event_handler = _http_event_handler,
        .user_data = response_buffer,
        .timeout_ms = 20000, // Increased timeout
        .is_async = false,
        .skip_cert_common_name_check = false,
        .use_global_ca_store = true,
        .crt_bundle_attach = esp_crt_bundle_attach,
        .transport_type = HTTP_TRANSPORT_OVER_SSL,
    };
    
    esp_http_client_handle_t client = esp_http_client_init(&config);
    if (!client) {
        ESP_LOGE(TAG, "Failed to init HTTP client for auto drive");
        free(base64_image);
        free(response_buffer);
        return ESP_FAIL;
    }
    
    // 1. 创建支持function calls的请求
    cJSON *json = cJSON_CreateObject();
    cJSON_AddStringToObject(json, "model", AI_MODEL);
    cJSON_AddNumberToObject(json, "max_tokens", 256); // Increased max_tokens
    cJSON_AddNumberToObject(json, "temperature", 0.7);
    cJSON_AddNumberToObject(json, "top_p", 0.7);
    cJSON_AddNumberToObject(json, "frequency_penalty", 0.5);
    cJSON_AddNumberToObject(json, "n", 1);
    
    // 2. 定义工具函数 (function calls)
    cJSON *tools = cJSON_CreateArray();
    cJSON *motor_tool = cJSON_CreateObject();
    cJSON_AddStringToObject(motor_tool, "type", "function");
    
    cJSON *function = cJSON_CreateObject();
    cJSON_AddStringToObject(function, "name", "control_motor");
    cJSON_AddStringToObject(function, "description", "根据视觉分析控制机器人移动。必须先提供思考过程再决定动作。");
    
    cJSON *parameters = cJSON_CreateObject();
    cJSON_AddStringToObject(parameters, "type", "object");
    cJSON *properties = cJSON_CreateObject();
    
    // **新增reasoning参数**
    cJSON *reasoning_param = cJSON_CreateObject();
    cJSON_AddStringToObject(reasoning_param, "type", "string");
    cJSON_AddStringToObject(reasoning_param, "description", "对当前画面的详细分析，以及基于分析得出的决策理由。");
    cJSON_AddItemToObject(properties, "reasoning", reasoning_param);

    cJSON *action_param = cJSON_CreateObject();
    cJSON_AddStringToObject(action_param, "type", "string");
    cJSON_AddStringToObject(action_param, "description", "Motor action: forward, backward, left, right, or stop");
    cJSON *enum_values = cJSON_CreateArray();
    cJSON_AddItemToArray(enum_values, cJSON_CreateString("forward"));
    cJSON_AddItemToArray(enum_values, cJSON_CreateString("backward")); 
    cJSON_AddItemToArray(enum_values, cJSON_CreateString("left"));
    cJSON_AddItemToArray(enum_values, cJSON_CreateString("right"));
    cJSON_AddItemToArray(enum_values, cJSON_CreateString("stop"));
    cJSON_AddItemToObject(action_param, "enum", enum_values);
    cJSON_AddItemToObject(properties, "action", action_param);
    
    cJSON *duration_param = cJSON_CreateObject();
    cJSON_AddStringToObject(duration_param, "type", "number");
    cJSON_AddStringToObject(duration_param, "description", "Duration in seconds (0.5-3.0)");
    cJSON_AddNumberToObject(duration_param, "minimum", 0.5);
    cJSON_AddNumberToObject(duration_param, "maximum", 3.0);
    cJSON_AddItemToObject(properties, "duration", duration_param);
    
    cJSON *speed_param = cJSON_CreateObject();
    cJSON_AddStringToObject(speed_param, "type", "integer");
    cJSON_AddStringToObject(speed_param, "description", "Motor speed (30-100)");
    cJSON_AddNumberToObject(speed_param, "minimum", 30);
    cJSON_AddNumberToObject(speed_param, "maximum", 100);
    cJSON_AddItemToObject(properties, "speed", speed_param);
    
    cJSON *required = cJSON_CreateArray();
    // **将reasoning设为必需**
    cJSON_AddItemToArray(required, cJSON_CreateString("reasoning"));
    cJSON_AddItemToArray(required, cJSON_CreateString("action"));
    cJSON_AddItemToArray(required, cJSON_CreateString("duration"));
    cJSON_AddItemToArray(required, cJSON_CreateString("speed"));
    
    cJSON_AddItemToObject(parameters, "properties", properties);
    cJSON_AddItemToObject(parameters, "required", required);
    cJSON_AddItemToObject(function, "parameters", parameters);
    cJSON_AddItemToObject(motor_tool, "function", function);
    cJSON_AddItemToArray(tools, motor_tool);
    cJSON_AddItemToObject(json, "tools", tools);
    
    cJSON_AddStringToObject(json, "tool_choice", "auto");
    
    cJSON *messages = cJSON_CreateArray();
    cJSON *message = cJSON_CreateObject();
    cJSON_AddStringToObject(message, "role", "user");
    
    cJSON *content = cJSON_CreateArray();
    cJSON *text_content = cJSON_CreateObject();
    cJSON_AddStringToObject(text_content, "type", "text");
    // 3. 更新Prompt，强制要求AI填充reasoning字段
    cJSON_AddStringToObject(text_content, "text", 
        "你是一个智能驾驶AI，控制一个机器人。你的任务是分析摄像头看到的实时画面，并决定下一步的动作。\n\n" 
        "**决策流程:**\n" 
        "1.  **观察 (Observe):** 详细描述你看到的关键事物、障碍物、空间布局和潜在的探索路径。\n" 
        "2.  **思考 (Think):** 基于你的观察，分析当前情况。如果前方有路，就前进。如果被挡住，就思考向左还是向右更开阔。如果看到有趣的东西，就说明为什么它有趣。\n" 
        "3.  **决策 (Decide):** 根据你的思考，选择一个最合适的动作 (action)。\n\n" 
        "**重要指令:** 你必须使用 `control_motor` 工具来执行你的决策。在调用工具时，**必须**在 `reasoning` 参数中完整地填写你的观察和思考过程，然后再确定 `action`, `duration`, 和 `speed`。\n\n" 
        "**示例:**\n" 
        "- reasoning: '我看到前方是一堵白墙，完全挡住了去路。左边看起来空间更大，所以我决定向左转来绕过障碍。' action: 'left'\n" 
        "- reasoning: '前方道路通畅，远处好像有一个红色的物体，我打算前进看清楚一点。' action: 'forward'\n\n" 
        "现在，请分析你看到的画面，并使用 `control_motor` 工具做出你的决策。"
    );
    cJSON_AddItemToArray(content, text_content);
    
    cJSON *image_content = cJSON_CreateObject();
    cJSON_AddStringToObject(image_content, "type", "image_url");
    cJSON *image_url = cJSON_CreateObject();
    char *image_url_str = malloc(strlen("data:image/jpeg;base64,") + strlen(base64_image) + 1);
    sprintf(image_url_str, "data:image/jpeg;base64,%s", base64_image);
    cJSON_AddStringToObject(image_url, "url", image_url_str);
    cJSON_AddItemToObject(image_content, "image_url", image_url);
    cJSON_AddItemToArray(content, image_content);
    
    cJSON_AddItemToObject(message, "content", content);
    cJSON_AddItemToArray(messages, message);
    cJSON_AddItemToObject(json, "messages", messages);
    
    char *json_string = cJSON_Print(json);
    free(base64_image);
    free(image_url_str);
    
    ESP_LOGI(TAG, "📡 准备发送AI自动驾驶请求");
    ESP_LOGI(TAG, "🔧 Tool Call工具已配置: control_motor (with reasoning)");
    
    char auth_header[256];
    snprintf(auth_header, sizeof(auth_header), "Bearer %s", AI_API_KEY);
    
    esp_http_client_set_header(client, "Authorization", auth_header);
    esp_http_client_set_header(client, "Content-Type", "application/json");
    esp_http_client_set_post_field(client, json_string, strlen(json_string));
    
    ESP_LOGI(TAG, "🌐 开始执行HTTP请求...");
    esp_err_t err = esp_http_client_perform(client);
    
    if (err == ESP_OK) {
        int status_code = esp_http_client_get_status_code(client);
        ESP_LOGI(TAG, "AI自动驾驶HTTP状态: %d", status_code);
        
        if (status_code == 200) {
            ESP_LOGI(TAG, "开始解析AI自动驾驶响应...");
            
            cJSON *response_json = cJSON_Parse(response_buffer);
            if (response_json) {
                cJSON *choices = cJSON_GetObjectItem(response_json, "choices");
                if (cJSON_IsArray(choices) && cJSON_GetArraySize(choices) > 0) {
                    cJSON *choice = cJSON_GetArrayItem(choices, 0);
                    cJSON *message = cJSON_GetObjectItem(choice, "message");
                    cJSON *tool_calls = cJSON_GetObjectItem(message, "tool_calls");
                    
                    if (cJSON_IsArray(tool_calls) && cJSON_GetArraySize(tool_calls) > 0) {
                        ESP_LOGI(TAG, "✅ AI使用了Tool Call!");
                        
                        cJSON *tool_call = cJSON_GetArrayItem(tool_calls, 0);
                        cJSON *function = cJSON_GetObjectItem(tool_call, "function");
                        cJSON *arguments = cJSON_GetObjectItem(function, "arguments");
                        
                        if (cJSON_IsString(arguments)) {
                            ESP_LOGI(TAG, "📋 Tool Call参数: %s", cJSON_GetStringValue(arguments));
                            
                            cJSON *args_json = cJSON_Parse(cJSON_GetStringValue(arguments));
                            if (args_json) {
                                // 4. 解析新的reasoning字段和原有字段
                                cJSON *reasoning = cJSON_GetObjectItem(args_json, "reasoning");
                                cJSON *action = cJSON_GetObjectItem(args_json, "action");
                                cJSON *duration = cJSON_GetObjectItem(args_json, "duration");
                                cJSON *speed = cJSON_GetObjectItem(args_json, "speed");
                                
                                if (cJSON_IsString(reasoning) && cJSON_IsString(action) && cJSON_IsNumber(duration) && cJSON_IsNumber(speed)) {
                                    const char *reasoning_str = cJSON_GetStringValue(reasoning);
                                    const char *action_str = cJSON_GetStringValue(action);
                                    double duration_val = cJSON_GetNumberValue(duration);
                                    int speed_val = (int)cJSON_GetNumberValue(speed);
                                    
                                    ESP_LOGI(TAG, "🧠 AI思考: %s", reasoning_str);
                                    ESP_LOGI(TAG, "🚗 AI驾驶决策: 动作=%s, 持续时间=%.1f秒, 速度=%d%%", 
                                        action_str, duration_val, speed_val);
                                    
                                    // 执行电机控制
                                    esp_err_t motor_result = ESP_FAIL;
                                    if (strcmp(action_str, "forward") == 0) motor_result = motor_forward(speed_val);
                                    else if (strcmp(action_str, "backward") == 0) motor_result = motor_backward(speed_val);
                                    else if (strcmp(action_str, "left") == 0) motor_result = motor_left(speed_val);
                                    else if (strcmp(action_str, "right") == 0) motor_result = motor_right(speed_val);
                                    else if (strcmp(action_str, "stop") == 0) motor_result = motor_stop_all();
                                    
                                    if (motor_result == ESP_OK && duration_val > 0) {
                                        vTaskDelay(pdMS_TO_TICKS((int)(duration_val * 1000)));
                                        motor_stop_all();
                                        ESP_LOGI(TAG, "🛑 电机已停止");
                                    }
                                    
                                    // 5. 使用解析出的reasoning_str更新最终结果
                                    char full_ai_result[1024];
                                    snprintf(full_ai_result, sizeof(full_ai_result), 
                                        "🧠 AI思考过程:\n%s\n\n🚗 驾驶决策: %s (%.1f秒, 速度%d%%)", 
                                        reasoning_str, action_str, duration_val, speed_val);
                                    storage_manager_update_ai_result(filename, full_ai_result);
                                    
                                } else {
                                    ESP_LOGW(TAG, "⚠️ Tool Call参数格式错误或缺少字段");
                                    storage_manager_update_ai_result(filename, "AI决策参数错误");
                                }
                                cJSON_Delete(args_json);
                            } else {
                                ESP_LOGE(TAG, "❌ Tool Call参数JSON解析失败");
                                storage_manager_update_ai_result(filename, "AI响应解析失败");
                            }
                        } else {
                            ESP_LOGW(TAG, "⚠️ Tool Call缺少参数");
                            storage_manager_update_ai_result(filename, "AI响应缺少参数");
                        }
                    } else {
                        ESP_LOGW(TAG, "❌ AI没有使用Tool Call，检查Prompt或模型能力");
                        cJSON *content = cJSON_GetObjectItem(message, "content");
                        if (cJSON_IsString(content)) {
                            storage_manager_update_ai_result(filename, cJSON_GetStringValue(content));
                        } else {
                            storage_manager_update_ai_result(filename, "AI未按预期返回工具调用");
                        }
                    }
                } else {
                    ESP_LOGE(TAG, "❌ AI响应中没有找到choices字段");
                    storage_manager_update_ai_result(filename, "AI响应格式错误 (no choices)");
                }
                cJSON_Delete(response_json);
            } else {
                ESP_LOGE(TAG, "❌ AI响应JSON解析失败，原始响应: %.200s", response_buffer);
                storage_manager_update_ai_result(filename, "AI响应JSON解析失败");
            }
            socket_failure_count = 0;
        } else {
            ESP_LOGW(TAG, "❌ AI自动驾驶API请求失败，状态码: %d", status_code);
            ESP_LOGW(TAG, "错误响应内容: %.200s", response_buffer);
            storage_manager_update_ai_result(filename, "AI服务API请求失败");
            socket_failure_count++;
        }
    } else {
        ESP_LOGE(TAG, "❌ AI自动驾驶HTTP请求失败: %s", esp_err_to_name(err));
        storage_manager_update_ai_result(filename, "AI服务HTTP请求失败");
        socket_failure_count++;
    }
    
    esp_http_client_cleanup(client);
    cJSON_Delete(json);
    free(json_string);
    free(response_buffer);
    return err;
}


// AI命令驱动分析函数
esp_err_t ai_service_command_analyze(const char* command)
{
    ESP_LOGI(TAG, "🗣️ 启动AI命令分析功能");
    ESP_LOGI(TAG, "📝 用户指令: %s", command);
    
    if (socket_failure_count >= 10) {
        ESP_LOGW(TAG, "Socket失败次数过多，暂停AI命令分析 10 秒");
        vTaskDelay(pdMS_TO_TICKS(10000));
        socket_failure_count = 0;
        return ESP_FAIL;
    }
    
    if (!wifi_manager_is_sta_connected()) {
        ESP_LOGW(TAG, "WiFi未连接，跳过AI命令分析");
        socket_failure_count++;
        return ESP_FAIL;
    }
    
    size_t free_heap = esp_get_free_heap_size();
    ESP_LOGI(TAG, "命令AI可用内存: %u 字节", (unsigned int)free_heap);
    if (free_heap < 50000) {
        ESP_LOGW(TAG, "内存不足，跳过AI命令分析");
        socket_failure_count++;
        return ESP_FAIL;
    }
    
    // 获取当前摄像头图像
    camera_fb_t *fb = NULL;
    esp_err_t capture_result = camera_capture(&fb);
    if (capture_result != ESP_OK || !fb) {
        ESP_LOGE(TAG, "无法获取摄像头图像");
        return ESP_FAIL;
    }
    
    // 执行AI命令分析
    esp_err_t result = ai_service_execute_command_with_image(fb, "command_analysis", command);
    
    // 释放图像缓冲区
    camera_return_fb(fb);
    
    return result;
}

// AI物体搜索任务实现
esp_err_t ai_service_start_object_search(const char* target_object, int timeout_seconds, bool use_navigation)
{
    ESP_LOGI(TAG, "🔍 启动AI物体搜索任务");
    ESP_LOGI(TAG, "🎯 目标物体: %s, 超时: %d秒, 导航: %s", 
        target_object, timeout_seconds, use_navigation ? "开启" : "关闭");
    
    // 检查网络连接
    if (!wifi_manager_is_sta_connected()) {
        ESP_LOGW(TAG, "WiFi未连接，无法启动AI物体搜索");
        return ESP_FAIL;
    }
    
    // 检查内存
    size_t free_heap = esp_get_free_heap_size();
    if (free_heap < 60000) {
        ESP_LOGW(TAG, "内存不足，无法启动AI物体搜索");
        return ESP_FAIL;
    }
    
    // 启动本地AI搜索任务
    esp_err_t result = local_ai_start_navigation_task(target_object, timeout_seconds, use_navigation);
    
    if (result == ESP_OK) {
        ESP_LOGI(TAG, "✅ AI物体搜索任务启动成功");
        printf("\n=== AI物体搜索任务启动 ===\n");
        printf("🎯 目标物体: %s\n", target_object);
        printf("⏰ 超时时间: %d秒\n", timeout_seconds);
        printf("🧭 主动导航: %s\n", use_navigation ? "开启" : "关闭");
        printf("📊 任务状态: 正在搜索...\n");
        printf("==============================\n\n");
    } else {
        ESP_LOGE(TAG, "❌ AI物体搜索任务启动失败");
    }
    
    return result;
}

esp_err_t ai_service_stop_object_search(void)
{
    ESP_LOGI(TAG, "⏹️ 停止AI物体搜索任务");
    
    esp_err_t result = local_ai_stop_task();
    
    if (result == ESP_OK) {
        ESP_LOGI(TAG, "✅ AI物体搜索任务已停止");
        printf("\n=== AI物体搜索任务停止 ===\n");
        printf("📊 任务状态: 已停止\n");
        printf("==============================\n\n");
    } else {
        ESP_LOGE(TAG, "❌ 停止AI物体搜索任务失败");
    }
    
    return result;
}

const ai_task_t* ai_service_get_search_status(void)
{
    return local_ai_get_task_status();
}

esp_err_t ai_service_process_search_task(camera_fb_t *fb)
{
    if (!fb) {
        return ESP_FAIL;
    }
    
    // 获取当前任务状态
    const ai_task_t* task = local_ai_get_task_status();
    if (!task || task->status == AI_TASK_IDLE) {
        return ESP_OK; // 没有活动任务
    }
    
    // 处理本地AI搜索任务
    esp_err_t result = local_ai_process_task(fb);
    
    // 获取更新后的状态
    task = local_ai_get_task_status();
    if (task) {
        // 实时输出任务状态
        static ai_task_status_t last_status = AI_TASK_IDLE;
        static float last_progress = -1.0f;
        static time_t last_update = 0;
        time_t now = time(NULL);
        
        // 状态改变时或每5秒输出一次状态
        if (task->status != last_status || 
            fabs(task->progress - last_progress) > 0.1f ||
            (now - last_update) >= 5) {
            
            printf("\n=== AI搜索任务状态更新 ===\n");
            printf("🎯 目标物体: %s\n", task->target_object);
            
            switch (task->status) {
                case AI_TASK_IDLE:
                    printf("📊 状态: 空闲\n");
                    break;
                case AI_TASK_SEARCHING:
                    printf("📊 状态: 正在搜索\n");
                    break;
                case AI_TASK_SCANNING:
                    printf("📊 状态: 正在扫描\n");
                    break;
                case AI_TASK_MOVING:
                    printf("📊 状态: 正在移动搜索\n");
                    break;
                case AI_TASK_NAVIGATING:
                    printf("📊 状态: 正在导航\n");
                    break;
                case AI_TASK_COMPLETED:
                    printf("📊 状态: ✅ 任务完成\n");
                    if (task->target_found) {
                        printf("🎉 找到目标! 置信度: %.1f%%\n", task->target_confidence * 100);
                    }
                    break;
                case AI_TASK_FAILED_TIMEOUT:
                    printf("📊 状态: ⏰ 任务超时\n");
                    break;
                case AI_TASK_FAILED_UNABLE:
                    printf("📊 状态: ❌ 无法完成\n");
                    break;
            }
            
            printf("📈 进度: %.1f%%\n", task->progress * 100);
            printf("🔄 搜索周期: %d\n", task->search_cycles);
            printf("🗺️ 已探索区域: %d\n", task->areas_explored);
            printf("💬 状态信息: %s\n", task->status_message);
            
            if (strlen(task->detailed_log) > 0) {
                printf("📝 详细日志: %s\n", task->detailed_log);
            }
            
            printf("==============================\n\n");
            
            last_status = task->status;
            last_progress = task->progress;
            last_update = now;
        }
        
        // 如果任务完成，更新存储记录
        if (task->status == AI_TASK_COMPLETED || 
            task->status == AI_TASK_FAILED_TIMEOUT || 
            task->status == AI_TASK_FAILED_UNABLE) {
            
            char result_message[256];
            snprintf(result_message, sizeof(result_message),
                "🔍 物体搜索结果:\n目标: %s\n状态: %s\n进度: %.1f%%\n搜索周期: %d\n已探索区域: %d",
                task->target_object,
                task->status == AI_TASK_COMPLETED ? "成功完成" : 
                (task->status == AI_TASK_FAILED_TIMEOUT ? "超时失败" : "无法完成"),
                task->progress * 100,
                task->search_cycles,
                task->areas_explored);
            
            storage_manager_update_ai_result("object_search", result_message);
        }
    }
    
    return result;
}
