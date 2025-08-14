idf_component_register(SRCS "ai_service.c" "local_ai_service.c"
                    INCLUDE_DIRS "."
                    REQUIRES esp_http_client json mbedtls camera_driver storage_manager wifi_manager motor_driver esp_timer
                    PRIV_REQUIRES esp-tls)