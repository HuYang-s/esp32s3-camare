idf_component_register(SRCS "esp32s3_camera_web.c"
                    INCLUDE_DIRS "."
                    REQUIRES "camera_driver" "esp_http_client" "json" "mbedtls" "spiffs")
