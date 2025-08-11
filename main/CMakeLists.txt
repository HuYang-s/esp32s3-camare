idf_component_register(SRCS "esp32s3_camera_web.c"
                    INCLUDE_DIRS "."
                    REQUIRES
                    freertos
                    nvs_flash
                    idf::log
                    camera_driver
                    wifi_manager
                    time_service
                    storage_manager
                    ai_service
                    web_server
                    motor_driver)