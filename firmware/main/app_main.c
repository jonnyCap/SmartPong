#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "esp_system.h"
#include "nvs_flash.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "protocol_examples_common.h"
#include "esp_log.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "mqtt_logic.h"
#include "windowing_task.h"
#include "m5stickc_plus.h"
#include "app_types.h"

static const char *TAG = "app_main";

static m5stickc_plus_t m5_device;
static app_display_snapshot_t display_snapshot = {0};

static void display_task(void *arg)
{
    display_snapshot.config_valid = true;
    display_snapshot.wifi_connected = true;
    display_snapshot.tcp_connected = true;
    display_snapshot.recorder_state = APP_RECORDER_STATE_READY;
    snprintf(display_snapshot.server_host, sizeof(display_snapshot.server_host), "SmartPong");
    snprintf(display_snapshot.status_line, sizeof(display_snapshot.status_line), "SMARTPONG");
    snprintf(display_snapshot.detail_line, sizeof(display_snapshot.detail_line), "READY TO THROW");
    snprintf(display_snapshot.server_message, sizeof(display_snapshot.server_message), "WAITING");

    while (1) {
        float ax = 0, ay = 0, az = 0;
        if (m5stickc_plus_read_accel_g(&m5_device, &ax, &ay, &az) == ESP_OK) {
            display_snapshot.ax_g = ax;
            display_snapshot.ay_g = ay;
            display_snapshot.az_g = az;
        }
        m5stickc_plus_render_status(&m5_device, &display_snapshot);
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

#define BUTTON_HIT_GPIO 37 // Commonly Button A on M5StickC
#define BUTTON_MISS_GPIO 39 // Commonly Button B on M5StickC

static void IRAM_ATTR gpio_isr_handler(void* arg)
{
    uint32_t gpio_num = (uint32_t) arg;
    if (gpio_num == BUTTON_HIT_GPIO) {
        confirm_throw_result(true);
    } else if (gpio_num == BUTTON_MISS_GPIO) {
        confirm_throw_result(false);
    }
}

static void configure_buttons(void)
{
    gpio_config_t io_conf = {};
    io_conf.intr_type = GPIO_INTR_NEGEDGE;
    io_conf.pin_bit_mask = (1ULL<<BUTTON_HIT_GPIO) | (1ULL<<BUTTON_MISS_GPIO);
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pull_up_en = 1;
    gpio_config(&io_conf);

    gpio_install_isr_service(0);
    gpio_isr_handler_add((gpio_num_t)BUTTON_HIT_GPIO, gpio_isr_handler, (void*) BUTTON_HIT_GPIO);
    gpio_isr_handler_add((gpio_num_t)BUTTON_MISS_GPIO, gpio_isr_handler, (void*) BUTTON_MISS_GPIO);
}

void app_main(void)
{
    ESP_LOGI(TAG, "[APP] Startup SmartPong Firmware..");

    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    // Wait for WiFi
    ESP_ERROR_CHECK(example_connect());

    // Configure Hit/Miss inputs
    configure_buttons();

    // Initialize M5StickC Plus hardware
    ESP_ERROR_CHECK(m5stickc_plus_init(&m5_device));

    // Start screen rendering task
    xTaskCreate(display_task, "display_task", 4096, NULL, 4, NULL);

    // Start MQTT background processes
    mqtt_app_start();
    
    // Start the IMU Windowing and Classification Task
    start_windowing_task(&m5_device);
}
