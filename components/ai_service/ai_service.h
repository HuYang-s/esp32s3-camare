#ifndef AI_SERVICE_H
#define AI_SERVICE_H

#include "esp_err.h"
#include "camera_driver.h"

/**
 * @brief 使用AI服务分析给定的图像帧
 *
 * @param fb 指向图像帧缓冲区的指针
 * @param filename 图像关联的文件名 (用于更新历史记录)
 * @return esp_err_t
 */
esp_err_t ai_service_analyze_image(camera_fb_t *fb, const char* filename);

/**
 * @brief 使用AI分析图片并自动控制电机驾驶 (支持Tool Call)
 *
 * @param fb 指向图像帧缓冲区的指针
 * @param filename 图像关联的文件名 (用于更新历史记录)
 * @return esp_err_t
 */
esp_err_t ai_service_auto_drive_analyze(camera_fb_t *fb, const char* filename);

/**
 * @brief 根据用户命令执行AI图像分析任务
 *
 * @param command 用户输入的命令字符串
 * @return esp_err_t
 */
esp_err_t ai_service_command_analyze(const char* command);

/**
 * @brief 获取当前网络请求失败的计数值
 */
int ai_service_get_socket_failure_count(void);

/**
 * @brief 解析文本中的电机控制指令并执行
 * 当AI没有使用Tool Call但在文本中描述了control_motor时使用
 * 
 * @param text 包含电机控制指令的文本
 * @param filename 图像关联的文件名 (用于更新历史记录)
 * @return esp_err_t
 */
esp_err_t parse_text_motor_command(const char* text, const char* filename);

/**
 * @brief 使用AI根据用户命令分析图像
 * 用户可以通过网页发送自定义任务指令，AI将根据指令分析当前图像
 * 
 * @param command 用户的任务指令
 * @return esp_err_t
 */
esp_err_t ai_service_command_analyze(const char* command);

#endif // AI_SERVICE_H