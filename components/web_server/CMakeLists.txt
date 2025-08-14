idf_component_register(SRCS "web_server.c"
                    INCLUDE_DIRS "."
                    REQUIRES
                    esp_http_server
                    json
                    log
                    wifi_manager
                    time_service
                    storage_manager
                    motor_driver
                    camera_driver
                    ai_service)