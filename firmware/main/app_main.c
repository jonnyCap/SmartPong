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
app_display_snapshot_t display_snapshot = {0};

static void display_task(void *arg)
{
    // The display snapshot is seeded in app_main before this task starts so the
    // screen shows something immediately; here we only refresh live IMU data
    // and re-render. WiFi/connection status is owned by network_task.
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

// Connect WiFi in the background so a missing/slow network never blocks the UI.
// On success we start MQTT + throw classification; on failure the device stays
// usable and the screen shows an offline status instead of going black.
static void network_task(void *arg)
{
    snprintf(display_snapshot.detail_line, sizeof(display_snapshot.detail_line), "CONNECTING WIFI...");

    esp_err_t err = example_connect();
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "WiFi connect failed (%s) - continuing offline", esp_err_to_name(err));
        display_snapshot.wifi_connected = false;
        display_snapshot.tcp_connected = false;
        snprintf(display_snapshot.detail_line, sizeof(display_snapshot.detail_line), "NO WIFI - OFFLINE");
        vTaskDelete(NULL);
        return;
    }

    ESP_LOGI(TAG, "WiFi connected - starting MQTT and classification");
    display_snapshot.wifi_connected = true;
    display_snapshot.tcp_connected = true;
    snprintf(display_snapshot.detail_line, sizeof(display_snapshot.detail_line), "READY TO THROW");

    mqtt_app_start();
    start_windowing_task(&m5_device);

    vTaskDelete(NULL);
}

#define BUTTON_HIT_GPIO 37 // Commonly Button A on M5StickC
#define BUTTON_MISS_GPIO 39 // Commonly Button B on M5StickC

static void configure_buttons(void)
{
    gpio_config_t io_conf = {};
    io_conf.intr_type = GPIO_INTR_DISABLE;
    io_conf.pin_bit_mask = (1ULL<<BUTTON_HIT_GPIO) | (1ULL<<BUTTON_MISS_GPIO);
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pull_up_en = 0;
    gpio_config(&io_conf);
}

void app_main(void)
{
    ESP_LOGI(TAG, "[APP] Startup SmartPong Firmware..");

    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    // Configure Hit/Miss inputs
    configure_buttons();

    // Initialize M5StickC Plus hardware (AXP192 power, MPU6886 IMU, ST7789 LCD)
    // BEFORE networking so the screen comes up immediately and never sits black
    // while WiFi is connecting.
    ESP_ERROR_CHECK(m5stickc_plus_init(&m5_device));

    // Seed the display snapshot, then start rendering right away.
    display_snapshot.config_valid = true;
    display_snapshot.wifi_connected = false;
    display_snapshot.tcp_connected = false;
    display_snapshot.recorder_state = APP_RECORDER_STATE_READY;
    snprintf(display_snapshot.server_host, sizeof(display_snapshot.server_host), "SmartPong");
    snprintf(display_snapshot.status_line, sizeof(display_snapshot.status_line), "SMARTPONG");
    snprintf(display_snapshot.detail_line, sizeof(display_snapshot.detail_line), "BOOTING...");
    snprintf(display_snapshot.server_message, sizeof(display_snapshot.server_message), "WAITING");
    xTaskCreate(display_task, "display_task", 4096, NULL, 4, NULL);

    // Connect WiFi in the background (non-blocking, non-fatal); MQTT + the IMU
    // windowing/classification task start once WiFi is up.
    xTaskCreate(network_task, "network_task", 8192, NULL, 5, NULL);
}
