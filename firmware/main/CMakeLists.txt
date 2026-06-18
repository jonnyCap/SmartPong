idf_component_register(SRCS "app_main.c" "feature_extraction.cpp" "mqtt_logic.c" "windowing_task.cpp" "m5stickc_plus.c"
                    PRIV_REQUIRES mqtt nvs_flash esp_netif json esp_lcd esp_driver_gpio esp_driver_i2c esp_driver_spi
                    INCLUDE_DIRS ".")
