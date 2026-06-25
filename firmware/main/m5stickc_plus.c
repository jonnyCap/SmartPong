#include "m5stickc_plus.h"

#include <ctype.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "esp_check.h"
#include "esp_heap_caps.h"
#include "esp_lcd_panel_vendor.h"
#include "esp_log.h"

#include "board_config.h"

#define I2C_TIMEOUT_MS                  1000
#define MPU6886_WHOAMI                  0x75
#define MPU6886_PWR_MGMT_1              0x6B
#define MPU6886_SMPLRT_DIV              0x19
#define MPU6886_CONFIG                  0x1A
#define MPU6886_GYRO_CONFIG             0x1B
#define MPU6886_ACCEL_CONFIG            0x1C
#define MPU6886_ACCEL_CONFIG2           0x1D
#define MPU6886_FIFO_EN                 0x23
#define MPU6886_INT_PIN_CFG             0x37
#define MPU6886_INT_ENABLE              0x38
#define MPU6886_USER_CTRL               0x6A
#define MPU6886_ACCEL_XOUT_H            0x3B
#define MPU6886_GYRO_XOUT_H             0x43

#define APP_COLOR_BACKGROUND            0x08A3
#define APP_COLOR_HEADER                0x11A8
#define APP_COLOR_PANEL                 0x19A7
#define APP_COLOR_PANEL_ALT             0x2146
#define APP_COLOR_TEXT                  0xFFFF
#define APP_COLOR_TEXT_DIM              0xBDF7
#define APP_COLOR_GOOD                  0x3F2D
#define APP_COLOR_BAD                   0xD1A7
#define APP_COLOR_ACCENT                0xFD20
#define APP_COLOR_RECORD                0xF8E7
#define APP_COLOR_TRACK                 0x3186

static const char *TAG = "m5stickc_plus";
static const float MPU6886_ACCEL_RES_G = 8.0f / 32768.0f;
static const float MPU6886_GYRO_RES_DPS = 2000.0f / 32768.0f;

typedef struct {
    char character;
    uint8_t rows[6];
} glyph_t;

static const glyph_t FONT_4X6[] = {
    {' ', {0x0, 0x0, 0x0, 0x0, 0x0, 0x0}},
    {'+', {0x0, 0x4, 0x4, 0xE, 0x4, 0x4}},
    {'-', {0x0, 0x0, 0x0, 0xE, 0x0, 0x0}},
    {'.', {0x0, 0x0, 0x0, 0x0, 0x6, 0x6}},
    {'/', {0x1, 0x1, 0x2, 0x4, 0x8, 0x8}},
    {':', {0x0, 0x6, 0x6, 0x0, 0x6, 0x6}},
    {'0', {0x6, 0x9, 0x9, 0x9, 0x9, 0x6}},
    {'1', {0x2, 0x6, 0x2, 0x2, 0x2, 0x7}},
    {'2', {0x6, 0x9, 0x1, 0x2, 0x4, 0xF}},
    {'3', {0xE, 0x1, 0x6, 0x1, 0x9, 0x6}},
    {'4', {0x1, 0x3, 0x5, 0x9, 0xF, 0x1}},
    {'5', {0xF, 0x8, 0xE, 0x1, 0x9, 0x6}},
    {'6', {0x6, 0x8, 0xE, 0x9, 0x9, 0x6}},
    {'7', {0xF, 0x1, 0x2, 0x4, 0x4, 0x4}},
    {'8', {0x6, 0x9, 0x6, 0x9, 0x9, 0x6}},
    {'9', {0x6, 0x9, 0x9, 0x7, 0x1, 0x6}},
    {'A', {0x6, 0x9, 0x9, 0xF, 0x9, 0x9}},
    {'B', {0xE, 0x9, 0xE, 0x9, 0x9, 0xE}},
    {'C', {0x6, 0x9, 0x8, 0x8, 0x9, 0x6}},
    {'D', {0xE, 0x9, 0x9, 0x9, 0x9, 0xE}},
    {'E', {0xF, 0x8, 0xE, 0x8, 0x8, 0xF}},
    {'F', {0xF, 0x8, 0xE, 0x8, 0x8, 0x8}},
    {'G', {0x6, 0x9, 0x8, 0xB, 0x9, 0x7}},
    {'H', {0x9, 0x9, 0xF, 0x9, 0x9, 0x9}},
    {'I', {0x7, 0x2, 0x2, 0x2, 0x2, 0x7}},
    {'K', {0x9, 0xA, 0xC, 0xC, 0xA, 0x9}},
    {'L', {0x8, 0x8, 0x8, 0x8, 0x8, 0xF}},
    {'M', {0x9, 0xF, 0xF, 0x9, 0x9, 0x9}},
    {'N', {0x9, 0xD, 0xD, 0xB, 0xB, 0x9}},
    {'O', {0x6, 0x9, 0x9, 0x9, 0x9, 0x6}},
    {'P', {0xE, 0x9, 0x9, 0xE, 0x8, 0x8}},
    {'R', {0xE, 0x9, 0x9, 0xE, 0xA, 0x9}},
    {'S', {0x7, 0x8, 0x6, 0x1, 0x1, 0xE}},
    {'T', {0xF, 0x2, 0x2, 0x2, 0x2, 0x2}},
    {'U', {0x9, 0x9, 0x9, 0x9, 0x9, 0x6}},
    {'V', {0x9, 0x9, 0x9, 0x9, 0x6, 0x6}},
    {'W', {0x9, 0x9, 0x9, 0xF, 0xF, 0x9}},
    {'X', {0x9, 0x9, 0x6, 0x6, 0x9, 0x9}},
    {'Y', {0x9, 0x9, 0x6, 0x2, 0x2, 0x2}},
    {'Z', {0xF, 0x1, 0x2, 0x4, 0x8, 0xF}},
};

static esp_err_t i2c_write_register(i2c_master_dev_handle_t device, uint8_t reg, uint8_t value)
{
    uint8_t payload[2] = {reg, value};
    return i2c_master_transmit(device, payload, sizeof(payload), I2C_TIMEOUT_MS);
}

static esp_err_t i2c_read_registers(i2c_master_dev_handle_t device, uint8_t reg, uint8_t *buffer, size_t length)
{
    return i2c_master_transmit_receive(device, &reg, 1, buffer, length, I2C_TIMEOUT_MS);
}

static esp_err_t i2c_read_register(i2c_master_dev_handle_t device, uint8_t reg, uint8_t *value)
{
    return i2c_read_registers(device, reg, value, 1);
}

static esp_err_t axp192_init(i2c_master_dev_handle_t axp192_dev)
{
    uint8_t value = 0;

    ESP_RETURN_ON_ERROR(i2c_write_register(axp192_dev, 0x28, 0xCC), TAG, "set ldo voltage failed");
    ESP_RETURN_ON_ERROR(i2c_write_register(axp192_dev, 0x82, 0xFF), TAG, "enable adc failed");
    ESP_RETURN_ON_ERROR(i2c_write_register(axp192_dev, 0x33, 0xC0), TAG, "set charge voltage failed");
    ESP_RETURN_ON_ERROR(i2c_read_register(axp192_dev, 0x12, &value), TAG, "read power register failed");
    ESP_RETURN_ON_ERROR(i2c_write_register(axp192_dev, 0x12, value | 0x4D), TAG, "enable power rails failed");
    ESP_RETURN_ON_ERROR(i2c_write_register(axp192_dev, 0x36, 0x0C), TAG, "set button timing failed");
    ESP_RETURN_ON_ERROR(i2c_write_register(axp192_dev, 0x91, 0xF0), TAG, "set rtc voltage failed");
    ESP_RETURN_ON_ERROR(i2c_write_register(axp192_dev, 0x90, 0x02), TAG, "set gpio0 failed");
    ESP_RETURN_ON_ERROR(i2c_write_register(axp192_dev, 0x30, 0x80), TAG, "disable vbus hold failed");
    ESP_RETURN_ON_ERROR(i2c_write_register(axp192_dev, 0x39, 0xFC), TAG, "set thermal protection failed");
    ESP_RETURN_ON_ERROR(i2c_write_register(axp192_dev, 0x35, 0xA2), TAG, "enable rtc charge failed");
    ESP_RETURN_ON_ERROR(i2c_write_register(axp192_dev, 0x32, 0x46), TAG, "enable battery detect failed");

    return ESP_OK;
}

static esp_err_t mpu6886_init(i2c_master_dev_handle_t mpu6886_dev)
{
    uint8_t whoami = 0;

    ESP_RETURN_ON_ERROR(i2c_read_register(mpu6886_dev, MPU6886_WHOAMI, &whoami), TAG, "read mpu6886 whoami failed");
    ESP_RETURN_ON_FALSE(whoami == 0x19, ESP_ERR_INVALID_RESPONSE, TAG, "unexpected mpu6886 whoami: 0x%02X", whoami);

    ESP_RETURN_ON_ERROR(i2c_write_register(mpu6886_dev, MPU6886_PWR_MGMT_1, 0x00), TAG, "wake imu failed");
    vTaskDelay(pdMS_TO_TICKS(10));
    ESP_RETURN_ON_ERROR(i2c_write_register(mpu6886_dev, MPU6886_PWR_MGMT_1, 0x80), TAG, "reset imu failed");
    vTaskDelay(pdMS_TO_TICKS(10));
    ESP_RETURN_ON_ERROR(i2c_write_register(mpu6886_dev, MPU6886_PWR_MGMT_1, 0x01), TAG, "set imu clock failed");
    vTaskDelay(pdMS_TO_TICKS(10));
    ESP_RETURN_ON_ERROR(i2c_write_register(mpu6886_dev, MPU6886_ACCEL_CONFIG, 0x10), TAG, "set accel range failed");
    ESP_RETURN_ON_ERROR(i2c_write_register(mpu6886_dev, MPU6886_GYRO_CONFIG, 0x18), TAG, "set gyro range failed");
    ESP_RETURN_ON_ERROR(i2c_write_register(mpu6886_dev, MPU6886_CONFIG, 0x01), TAG, "set dlpf failed");
    ESP_RETURN_ON_ERROR(i2c_write_register(mpu6886_dev, MPU6886_SMPLRT_DIV, 0x05), TAG, "set sample divider failed");
    ESP_RETURN_ON_ERROR(i2c_write_register(mpu6886_dev, MPU6886_INT_ENABLE, 0x00), TAG, "disable interrupts failed");
    ESP_RETURN_ON_ERROR(i2c_write_register(mpu6886_dev, MPU6886_ACCEL_CONFIG2, 0x00), TAG, "set accel lpf failed");
    ESP_RETURN_ON_ERROR(i2c_write_register(mpu6886_dev, MPU6886_USER_CTRL, 0x00), TAG, "disable user ctrl failed");
    ESP_RETURN_ON_ERROR(i2c_write_register(mpu6886_dev, MPU6886_FIFO_EN, 0x00), TAG, "disable fifo failed");
    ESP_RETURN_ON_ERROR(i2c_write_register(mpu6886_dev, MPU6886_INT_PIN_CFG, 0x22), TAG, "set interrupt pin failed");
    ESP_RETURN_ON_ERROR(i2c_write_register(mpu6886_dev, MPU6886_INT_ENABLE, 0x01), TAG, "enable data ready failed");
    vTaskDelay(pdMS_TO_TICKS(100));

    return ESP_OK;
}

static esp_err_t init_i2c_devices(m5stickc_plus_t *device)
{
    i2c_master_bus_config_t bus_config = {
        .i2c_port = M5STICKC_PLUS_I2C_PORT,
        .sda_io_num = M5STICKC_PLUS_I2C_SDA_GPIO,
        .scl_io_num = M5STICKC_PLUS_I2C_SCL_GPIO,
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = true,
    };
    i2c_device_config_t axp192_config = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = M5STICKC_PLUS_AXP192_I2C_ADDRESS,
        .scl_speed_hz = 400000,
    };
    i2c_device_config_t mpu6886_config = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = M5STICKC_PLUS_MPU6886_I2C_ADDRESS,
        .scl_speed_hz = 400000,
    };

    ESP_RETURN_ON_ERROR(i2c_new_master_bus(&bus_config, &device->i2c_bus), TAG, "create i2c bus failed");
    ESP_RETURN_ON_ERROR(i2c_master_bus_add_device(device->i2c_bus, &axp192_config, &device->axp192_dev), TAG, "add axp192 failed");
    ESP_RETURN_ON_ERROR(i2c_master_bus_add_device(device->i2c_bus, &mpu6886_config, &device->mpu6886_dev), TAG, "add mpu6886 failed");

    return ESP_OK;
}

static esp_err_t init_display(m5stickc_plus_t *device)
{
    spi_bus_config_t bus_config = {
        .sclk_io_num = M5STICKC_PLUS_LCD_SCLK_GPIO,
        .mosi_io_num = M5STICKC_PLUS_LCD_MOSI_GPIO,
        .miso_io_num = -1,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = M5STICKC_PLUS_LCD_H_RES * M5STICKC_PLUS_LCD_V_RES * (int)sizeof(uint16_t),
    };
    esp_lcd_panel_io_spi_config_t io_config = {
        .dc_gpio_num = M5STICKC_PLUS_LCD_DC_GPIO,
        .cs_gpio_num = M5STICKC_PLUS_LCD_CS_GPIO,
        .pclk_hz = M5STICKC_PLUS_LCD_PIXEL_CLOCK_HZ,
        .lcd_cmd_bits = 8,
        .lcd_param_bits = 8,
        .spi_mode = 0,
        .trans_queue_depth = 10,
    };
    esp_lcd_panel_dev_config_t panel_config = {
        .reset_gpio_num = M5STICKC_PLUS_LCD_RST_GPIO,
        .rgb_ele_order = LCD_RGB_ELEMENT_ORDER_BGR,
        .bits_per_pixel = 16,
    };
    size_t framebuffer_size = M5STICKC_PLUS_LCD_H_RES * M5STICKC_PLUS_LCD_V_RES * sizeof(uint16_t);

    ESP_RETURN_ON_ERROR(spi_bus_initialize(M5STICKC_PLUS_LCD_HOST, &bus_config, SPI_DMA_CH_AUTO), TAG, "init spi bus failed");
    ESP_RETURN_ON_ERROR(esp_lcd_new_panel_io_spi((esp_lcd_spi_bus_handle_t)M5STICKC_PLUS_LCD_HOST, &io_config, &device->panel_io), TAG, "create panel io failed");
    ESP_RETURN_ON_ERROR(esp_lcd_new_panel_st7789(device->panel_io, &panel_config, &device->panel), TAG, "create st7789 panel failed");
    ESP_RETURN_ON_ERROR(esp_lcd_panel_reset(device->panel), TAG, "panel reset failed");
    ESP_RETURN_ON_ERROR(esp_lcd_panel_init(device->panel), TAG, "panel init failed");
    ESP_RETURN_ON_ERROR(esp_lcd_panel_invert_color(device->panel, true), TAG, "panel invert failed");
    ESP_RETURN_ON_ERROR(esp_lcd_panel_swap_xy(device->panel, false), TAG, "panel swap xy failed");
    ESP_RETURN_ON_ERROR(esp_lcd_panel_mirror(device->panel, false, false), TAG, "panel mirror failed");
    ESP_RETURN_ON_ERROR(esp_lcd_panel_set_gap(device->panel, M5STICKC_PLUS_LCD_GAP_X, M5STICKC_PLUS_LCD_GAP_Y), TAG, "panel set gap failed");
    ESP_RETURN_ON_ERROR(esp_lcd_panel_disp_on_off(device->panel, true), TAG, "panel display on failed");

    device->framebuffer = heap_caps_malloc(framebuffer_size, MALLOC_CAP_DMA | MALLOC_CAP_8BIT);
    ESP_RETURN_ON_FALSE(device->framebuffer != NULL, ESP_ERR_NO_MEM, TAG, "alloc framebuffer failed");
    memset(device->framebuffer, 0, framebuffer_size);

    return ESP_OK;
}

static inline void framebuffer_fill(m5stickc_plus_t *device, uint16_t color)
{
    size_t pixels = M5STICKC_PLUS_LCD_H_RES * M5STICKC_PLUS_LCD_V_RES;
    for (size_t i = 0; i < pixels; ++i) {
        device->framebuffer[i] = color;
    }
}

static inline void framebuffer_put_pixel(m5stickc_plus_t *device, int x, int y, uint16_t color)
{
    if (x < 0 || y < 0 || x >= M5STICKC_PLUS_LCD_H_RES || y >= M5STICKC_PLUS_LCD_V_RES) {
        return;
    }
    device->framebuffer[(y * M5STICKC_PLUS_LCD_H_RES) + x] = color;
}

static void framebuffer_fill_rect(m5stickc_plus_t *device, int x, int y, int width, int height, uint16_t color)
{
    for (int row = y; row < y + height; ++row) {
        for (int col = x; col < x + width; ++col) {
            framebuffer_put_pixel(device, col, row, color);
        }
    }
}

static void framebuffer_draw_outline_rect(m5stickc_plus_t *device, int x, int y, int width, int height, uint16_t color)
{
    framebuffer_fill_rect(device, x, y, width, 1, color);
    framebuffer_fill_rect(device, x, y + height - 1, width, 1, color);
    framebuffer_fill_rect(device, x, y, 1, height, color);
    framebuffer_fill_rect(device, x + width - 1, y, 1, height, color);
}

static const uint8_t *font_lookup(char character)
{
    char uppercase = (char)toupper((unsigned char)character);

    for (size_t i = 0; i < sizeof(FONT_4X6) / sizeof(FONT_4X6[0]); ++i) {
        if (FONT_4X6[i].character == uppercase) {
            return FONT_4X6[i].rows;
        }
    }

    return FONT_4X6[0].rows;
}

static void framebuffer_draw_char(m5stickc_plus_t *device, int x, int y, char character, int scale, uint16_t color)
{
    const uint8_t *rows = font_lookup(character);

    for (int row = 0; row < 6; ++row) {
        for (int col = 0; col < 4; ++col) {
            if (rows[row] & (1U << (3 - col))) {
                framebuffer_fill_rect(device, x + (col * scale), y + (row * scale), scale, scale, color);
            }
        }
    }
}

static int measure_text_width(const char *text, int scale)
{
    size_t length = strlen(text);
    if (length == 0) {
        return 0;
    }
    return (int)(length * ((4 * scale) + scale)) - scale;
}

static void framebuffer_draw_text(m5stickc_plus_t *device, int x, int y, const char *text, int scale, uint16_t color)
{
    int cursor = x;

    for (size_t i = 0; text[i] != '\0'; ++i) {
        framebuffer_draw_char(device, cursor, y, text[i], scale, color);
        cursor += (4 * scale) + scale;
    }
}

static void framebuffer_draw_centered_text(m5stickc_plus_t *device, int y, const char *text, int scale, uint16_t color)
{
    int width = measure_text_width(text, scale);
    int x = (M5STICKC_PLUS_LCD_H_RES - width) / 2;

    framebuffer_draw_text(device, x, y, text, scale, color);
}

static void framebuffer_draw_centered_text_in_rect(m5stickc_plus_t *device,
                                                   int x,
                                                   int y,
                                                   int width,
                                                   const char *text,
                                                   int scale,
                                                   uint16_t color)
{
    int text_width = measure_text_width(text, scale);
    int text_x = x + ((width - text_width) / 2);

    framebuffer_draw_text(device, text_x, y, text, scale, color);
}

static void framebuffer_draw_badge(m5stickc_plus_t *device,
                                   int x,
                                   int y,
                                   int width,
                                   const char *label,
                                   bool ok,
                                   uint16_t color)
{
    framebuffer_fill_rect(device, x, y, width, 18, ok ? color : APP_COLOR_BAD);
    framebuffer_draw_outline_rect(device, x, y, width, 18, APP_COLOR_TEXT);
    framebuffer_draw_centered_text_in_rect(device, x, y + 4, width, label, 1, APP_COLOR_TEXT);
}

static void framebuffer_draw_progress(m5stickc_plus_t *device, const app_display_snapshot_t *snapshot)
{
    int y = 126;
    char line[24] = {0};

    snprintf(line, sizeof(line), "THROWS: %lu", (unsigned long)snapshot->sample_count);
    framebuffer_draw_centered_text(device, y, line, 2, APP_COLOR_TEXT);
}

static void framebuffer_draw_axis_meter(m5stickc_plus_t *device,
                                        int y,
                                        const char *label,
                                        float value,
                                        uint16_t color)
{
    int x = 12;
    int width = 111;
    int center = x + (width / 2);
    int meter_y = y + 8;
    int meter_height = 10;
    int meter_width = width;
    int half_width = meter_width / 2;
    int bar_width = (int)(fminf(fabsf(value), 2.0f) / 2.0f * (float)(half_width - 2));
    char line[16] = {0};

    framebuffer_draw_text(device, x, y, label, 1, APP_COLOR_TEXT);
    snprintf(line, sizeof(line), "%+.2f", value);
    framebuffer_draw_text(device, 87, y, line, 1, APP_COLOR_TEXT_DIM);

    framebuffer_fill_rect(device, x, meter_y, meter_width, meter_height, APP_COLOR_TRACK);
    framebuffer_draw_outline_rect(device, x, meter_y, meter_width, meter_height, APP_COLOR_TEXT_DIM);
    framebuffer_fill_rect(device, center, meter_y - 1, 1, meter_height + 2, APP_COLOR_TEXT);

    if (bar_width <= 0) {
        return;
    }

    if (value >= 0.0f) {
        framebuffer_fill_rect(device, center + 1, meter_y + 1, bar_width, meter_height - 2, color);
    } else {
        framebuffer_fill_rect(device, center - bar_width, meter_y + 1, bar_width, meter_height - 2, color);
    }
}

static void framebuffer_draw_status_card(m5stickc_plus_t *device, const app_display_snapshot_t *snapshot)
{
    uint16_t state_color = APP_COLOR_ACCENT;
    int card_x = 6;
    int card_y = 62;
    int card_width = M5STICKC_PLUS_LCD_H_RES - 12;
    int safe_x = card_x + 10;
    int safe_width = card_width - 20;

    framebuffer_fill_rect(device, card_x, card_y, card_width, 58, APP_COLOR_PANEL);
    framebuffer_draw_outline_rect(device, card_x, card_y, card_width, 58, APP_COLOR_TEXT_DIM);

    if (snapshot->recorder_state == APP_RECORDER_STATE_RECORDING) {
        state_color = APP_COLOR_RECORD;
    } else if (snapshot->recorder_state == APP_RECORDER_STATE_READY) {
        state_color = APP_COLOR_GOOD;
    } else if (snapshot->recorder_state == APP_RECORDER_STATE_BLOCKED) {
        state_color = APP_COLOR_BAD;
    }

    framebuffer_fill_rect(device, safe_x, 68, safe_width, 14, state_color);
    framebuffer_draw_outline_rect(device, safe_x, 68, safe_width, 14, APP_COLOR_TEXT);
    framebuffer_draw_centered_text_in_rect(device, safe_x, 72, safe_width, snapshot->status_line, 1, APP_COLOR_TEXT);

    framebuffer_draw_centered_text_in_rect(device, safe_x, 98, safe_width, snapshot->detail_line, 1, APP_COLOR_TEXT_DIM);
}

esp_err_t m5stickc_plus_init(m5stickc_plus_t *device)
{
    memset(device, 0, sizeof(*device));

    ESP_RETURN_ON_ERROR(init_i2c_devices(device), TAG, "init i2c failed");
    ESP_RETURN_ON_ERROR(axp192_init(device->axp192_dev), TAG, "init axp192 failed");
    ESP_RETURN_ON_ERROR(mpu6886_init(device->mpu6886_dev), TAG, "init mpu6886 failed");
    ESP_RETURN_ON_ERROR(init_display(device), TAG, "init display failed");

    device->imu_ready = true;
    device->display_ready = true;

    return ESP_OK;
}

esp_err_t m5stickc_plus_read_accel_g(m5stickc_plus_t *device, float *ax_g, float *ay_g, float *az_g)
{
    uint8_t raw[6] = {0};
    int16_t ax = 0;
    int16_t ay = 0;
    int16_t az = 0;

    ESP_RETURN_ON_FALSE(device->imu_ready, ESP_ERR_INVALID_STATE, TAG, "imu not initialized");
    ESP_RETURN_ON_ERROR(i2c_read_registers(device->mpu6886_dev, MPU6886_ACCEL_XOUT_H, raw, sizeof(raw)), TAG, "read accel data failed");

    ax = (int16_t)((raw[0] << 8) | raw[1]);
    ay = (int16_t)((raw[2] << 8) | raw[3]);
    az = (int16_t)((raw[4] << 8) | raw[5]);

    *ax_g = ax * MPU6886_ACCEL_RES_G;
    *ay_g = ay * MPU6886_ACCEL_RES_G;
    *az_g = az * MPU6886_ACCEL_RES_G;

    return ESP_OK;
}

esp_err_t m5stickc_plus_read_gyro_dps(m5stickc_plus_t *device, float *gx_dps, float *gy_dps, float *gz_dps)
{
    uint8_t raw[6] = {0};
    int16_t gx = 0;
    int16_t gy = 0;
    int16_t gz = 0;

    ESP_RETURN_ON_FALSE(device->imu_ready, ESP_ERR_INVALID_STATE, TAG, "imu not initialized");
    ESP_RETURN_ON_ERROR(i2c_read_registers(device->mpu6886_dev, MPU6886_GYRO_XOUT_H, raw, sizeof(raw)), TAG, "read gyro data failed");

    gx = (int16_t)((raw[0] << 8) | raw[1]);
    gy = (int16_t)((raw[2] << 8) | raw[3]);
    gz = (int16_t)((raw[4] << 8) | raw[5]);

    *gx_dps = gx * MPU6886_GYRO_RES_DPS;
    *gy_dps = gy * MPU6886_GYRO_RES_DPS;
    *gz_dps = gz * MPU6886_GYRO_RES_DPS;

    return ESP_OK;
}

esp_err_t m5stickc_plus_render_status(m5stickc_plus_t *device, const app_display_snapshot_t *snapshot)
{
    char line[40] = {0};
    char server_line[32] = {0};

    ESP_RETURN_ON_FALSE(device->display_ready, ESP_ERR_INVALID_STATE, TAG, "display not initialized");

    framebuffer_fill(device, APP_COLOR_BACKGROUND);

    if (snapshot->recorder_state == APP_RECORDER_STATE_RECORDING) {
        framebuffer_fill_rect(device, 0, 0, M5STICKC_PLUS_LCD_H_RES, M5STICKC_PLUS_LCD_V_RES, 0x0000);
        framebuffer_draw_centered_text(device, 60, "THROW DETECTED", 2, APP_COLOR_TEXT);
        framebuffer_draw_centered_text(device, 110, "A: HIT (1x PRESS)", 1, APP_COLOR_TEXT);
        framebuffer_draw_centered_text(device, 130, "A: MISS (2x PRESS)", 1, APP_COLOR_TEXT);
        framebuffer_draw_centered_text(device, 160, "B (SIDE): MISS", 1, APP_COLOR_TEXT);
        framebuffer_draw_centered_text(device, 200, "(Timeout in 10s)", 1, APP_COLOR_TEXT_DIM);
    } else {
        framebuffer_fill_rect(device, 0, 0, M5STICKC_PLUS_LCD_H_RES, 30, APP_COLOR_HEADER);
        framebuffer_draw_centered_text(device, 8, "SMART PONG", 2, APP_COLOR_TEXT);

        framebuffer_draw_badge(device, 8, 38, 54, snapshot->wifi_connected ? "WIFI OK" : "WIFI WAIT", snapshot->wifi_connected, APP_COLOR_GOOD);
        framebuffer_draw_badge(device, 73, 38, 54, snapshot->tcp_connected ? "TCP OK" : "TCP WAIT", snapshot->tcp_connected, APP_COLOR_ACCENT);

        framebuffer_draw_status_card(device, snapshot);
        framebuffer_draw_progress(device, snapshot);

        framebuffer_fill_rect(device, 6, 148, M5STICKC_PLUS_LCD_H_RES - 12, 62, APP_COLOR_PANEL_ALT);
        framebuffer_draw_outline_rect(device, 6, 148, M5STICKC_PLUS_LCD_H_RES - 12, 62, APP_COLOR_TEXT_DIM);
        framebuffer_draw_axis_meter(device, 156, "AX", snapshot->ax_g, APP_COLOR_ACCENT);
        framebuffer_draw_axis_meter(device, 176, "AY", snapshot->ay_g, APP_COLOR_GOOD);
        framebuffer_draw_axis_meter(device, 196, "AZ", snapshot->az_g, APP_COLOR_RECORD);

        snprintf(server_line, sizeof(server_line), "%s", snapshot->server_message[0] != '\0' ? snapshot->server_message : "NO MESSAGE");
        framebuffer_draw_centered_text(device, 218, server_line, 1, APP_COLOR_TEXT);

        if (snapshot->wifi_connected) {
            snprintf(line, sizeof(line), "%s:%u", snapshot->server_host, snapshot->server_port);
        } else {
            snprintf(line, sizeof(line), "%s", snapshot->server_host);
        }
        framebuffer_draw_centered_text(device, 230, line, 1, APP_COLOR_TEXT_DIM);
    }

    return esp_lcd_panel_draw_bitmap(device->panel, 0, 0, M5STICKC_PLUS_LCD_H_RES, M5STICKC_PLUS_LCD_V_RES, device->framebuffer);
}
