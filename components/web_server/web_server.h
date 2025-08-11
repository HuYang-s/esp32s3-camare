#ifndef WEB_SERVER_H
#define WEB_SERVER_H

#include "esp_err.h"
#include <stdbool.h>

esp_err_t web_server_start(void);

/**
 * @brief 获取AI自动驾驶状态
 * @return true if AI auto drive is enabled, false otherwise
 */
bool web_server_get_auto_drive_status(void);

#endif // WEB_SERVER_H
