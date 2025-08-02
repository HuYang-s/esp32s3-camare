idf_component_register(
    SRCS "camera_driver.c"
    INCLUDE_DIRS "include"
    REQUIRES "esp32-camera" "esp_http_server" "esp_wifi" "nvs_flash" "esp_http_client" "json" "mbedtls"
)