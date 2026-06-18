idf_component_register(SRCS "app_main.c" "feature_extraction.cpp" "mqtt_logic.c" "windowing_task.cpp"
                    PRIV_REQUIRES mqtt nvs_flash esp_netif json
                    INCLUDE_DIRS ".")
