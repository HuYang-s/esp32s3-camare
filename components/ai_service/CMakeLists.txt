idf_component_register(SRCS "ai_service.c" "ai_service_text_parser.c"
                    INCLUDE_DIRS "."
                    REQUIRES esp_http_client json mbedtls idf::log wifi_manager storage_manager esp-tls motor_driver)