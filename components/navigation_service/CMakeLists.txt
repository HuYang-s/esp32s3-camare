idf_component_register(SRCS "navigation_service.c"
                    INCLUDE_DIRS "."
                    REQUIRES motor_driver camera_driver ai_service log
                    PRIV_REQUIRES freertos)