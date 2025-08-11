idf_component_register(SRCS "time_service.c"
                    INCLUDE_DIRS "."
                    REQUIRES esp_netif idf::log)