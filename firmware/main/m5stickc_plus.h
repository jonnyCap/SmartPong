#pragma once

#include <stdbool.h>

#include "driver/i2c_master.h"
#include "esp_err.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_ops.h"

#include "app_types.h"

typedef struct {
    i2c_master_bus_handle_t i2c_bus;
    i2c_master_dev_handle_t axp192_dev;
    i2c_master_dev_handle_t mpu6886_dev;
    esp_lcd_panel_io_handle_t panel_io;
    esp_lcd_panel_handle_t panel;
    uint16_t *framebuffer;
    bool imu_ready;
    bool display_ready;
} m5stickc_plus_t;

esp_err_t m5stickc_plus_init(m5stickc_plus_t *device);
esp_err_t m5stickc_plus_read_accel_g(m5stickc_plus_t *device, float *ax_g, float *ay_g, float *az_g);
esp_err_t m5stickc_plus_read_gyro_dps(m5stickc_plus_t *device, float *gx_dps, float *gy_dps, float *gz_dps);
esp_err_t m5stickc_plus_render_status(m5stickc_plus_t *device, const app_display_snapshot_t *snapshot);
