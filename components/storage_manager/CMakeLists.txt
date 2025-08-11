idf_component_register(SRCS "storage_manager.c"
                    INCLUDE_DIRS "."
                    REQUIRES spiffs idf::log camera_driver)