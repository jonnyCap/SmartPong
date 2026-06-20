#include "windowing_task.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_system.h"
#include "esp_random.h"
#include <string.h>

#include "feature_extraction.h"
#include "classifier/final_model.h"
#include "mqtt_logic.h"
#include "app_types.h"
#include "driver/gpio.h"

extern "C" {
    extern app_display_snapshot_t display_snapshot;
}

static const char *TAG = "windowing_task";

#define WINDOW_SIZE 128
#define STEP_SIZE 64
#define IMU_RATE_MS 20

// Simulated IMU buffers
static float ax_buf[WINDOW_SIZE];
static float ay_buf[WINDOW_SIZE];
static float az_buf[WINDOW_SIZE];
static float gx_buf[WINDOW_SIZE];
static float gy_buf[WINDOW_SIZE];
static float gz_buf[WINDOW_SIZE];

static volatile bool waiting_for_input = false;
static volatile bool hit_result = false;
static char user_id[32] = {0};

// Instance of the decision tree
static Eloquent::ML::Port::DecisionTree tree;

static void shift_buffer(float* buf, int shift_amount) {
    memmove(buf, buf + shift_amount, (WINDOW_SIZE - shift_amount) * sizeof(float));
}

static void read_imu_data(m5stickc_plus_t *device, int index) {
    float ax = 0.0f, ay = 0.0f, az = 0.0f;
    float gx = 0.0f, gy = 0.0f, gz = 0.0f;
    if (device) {
        m5stickc_plus_read_accel_g(device, &ax, &ay, &az);
        m5stickc_plus_read_gyro_dps(device, &gx, &gy, &gz);
    }
    ax_buf[index] = ax;
    ay_buf[index] = ay;
    az_buf[index] = az;
    gx_buf[index] = gx;
    gy_buf[index] = gy;
    gz_buf[index] = gz;
}

static void imu_task(void *pvParameters) {
    m5stickc_plus_t *device = (m5stickc_plus_t *)pvParameters;
    ESP_LOGI(TAG, "Starting IMU Windowing Task");

    int sample_idx = 0;

    while (1) {
        if (waiting_for_input) {
            // Wait until button sets waiting_for_input to false
            vTaskDelay(pdMS_TO_TICKS(100));
            continue;
        }

        read_imu_data(device, sample_idx);
        sample_idx++;

        if (sample_idx == WINDOW_SIZE) {
            // Window is full, extract features and classify
            float features[7];
            extract_features(ax_buf, ay_buf, az_buf, WINDOW_SIZE, features);

            int class_idx = tree.predict(features);
            
            if (class_idx == 1) { // 1 == Throwing from final_model.h
                ESP_LOGI(TAG, "THROW DETECTED! Waiting for Hit/Miss button input...");
                ESP_LOGI(TAG, "(Simulate by pressing GPIO37 for hit, GPIO39 for miss if configured)");
                
                display_snapshot.recorder_state = APP_RECORDER_STATE_RECORDING;
                snprintf(display_snapshot.status_line, sizeof(display_snapshot.status_line), "THROW DETECTED");
                snprintf(display_snapshot.detail_line, sizeof(display_snapshot.detail_line), "HIT OR MISS? (10s)");
                
                // Add a 1s delay so any accidental button presses during the physical throw are ignored
                vTaskDelay(pdMS_TO_TICKS(1000));
                
                waiting_for_input = true;

                // Wait until the user presses the button with a 10s timeout
                int timeout_counter = 0;
                while (waiting_for_input && timeout_counter < 200) {
                    if (gpio_get_level((gpio_num_t)39) == 0) {
                        hit_result = false;
                        waiting_for_input = false;
                    } else if (gpio_get_level((gpio_num_t)37) == 0) {
                        // Wait for release
                        while (gpio_get_level((gpio_num_t)37) == 0) {
                            vTaskDelay(pdMS_TO_TICKS(10));
                        }
                        
                        // Wait up to 400ms for a second press
                        int double_click_wait = 0;
                        bool second_press = false;
                        while (double_click_wait < 40) {
                            if (gpio_get_level((gpio_num_t)37) == 0) {
                                second_press = true;
                                break;
                            }
                            vTaskDelay(pdMS_TO_TICKS(10));
                            double_click_wait++;
                        }
                        
                        if (second_press) {
                            // Wait for release again
                            while (gpio_get_level((gpio_num_t)37) == 0) {
                                vTaskDelay(pdMS_TO_TICKS(10));
                            }
                            hit_result = false; // Double press = MISS
                        } else {
                            hit_result = true; // Single press = HIT
                        }
                        waiting_for_input = false;
                    }
                    
                    if (waiting_for_input) {
                        vTaskDelay(pdMS_TO_TICKS(50));
                        timeout_counter++;
                    }
                }
                
                if (waiting_for_input) {
                    ESP_LOGI(TAG, "Timeout waiting for user input. Defaulting to Miss.");
                    hit_result = false;
                    waiting_for_input = false;
                }

                ESP_LOGI(TAG, "User input received. Hit: %d. Sending to MQTT...", hit_result);
                
                // Publish MQTT
                mqtt_publish_throw_data(user_id, hit_result, ax_buf, ay_buf, az_buf, gx_buf, gy_buf, gz_buf, WINDOW_SIZE);
                
                display_snapshot.sample_count++;
                display_snapshot.recorder_state = APP_RECORDER_STATE_READY;
                snprintf(display_snapshot.status_line, sizeof(display_snapshot.status_line), "SMARTPONG");
                snprintf(display_snapshot.detail_line, sizeof(display_snapshot.detail_line), "READY TO THROW");

                // Clear the buffer completely to avoid double triggering on the tail end of the throw
                sample_idx = 0;
            } else {
                // Shift window for 50% overlap
                shift_buffer(ax_buf, STEP_SIZE);
                shift_buffer(ay_buf, STEP_SIZE);
                shift_buffer(az_buf, STEP_SIZE);
                shift_buffer(gx_buf, STEP_SIZE);
                shift_buffer(gy_buf, STEP_SIZE);
                shift_buffer(gz_buf, STEP_SIZE);
                sample_idx = WINDOW_SIZE - STEP_SIZE;
            }
        }

        vTaskDelay(pdMS_TO_TICKS(IMU_RATE_MS));
    }
}

void start_windowing_task(m5stickc_plus_t *device) {
    if (user_id[0] == 0) {
        snprintf(user_id, sizeof(user_id), "user_%lu", (unsigned long)esp_random());
        ESP_LOGI(TAG, "Generated device UUID: %s", user_id);
    }
    xTaskCreate(imu_task, "imu_task", 8192, device, 5, NULL);
}
