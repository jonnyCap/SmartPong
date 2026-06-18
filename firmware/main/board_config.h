#pragma once

#include "driver/i2c_master.h"
#include "driver/spi_master.h"

#define M5STICKC_PLUS_LED_GPIO                10
#define M5STICKC_PLUS_BUTTON_A_GPIO           37
#define M5STICKC_PLUS_BUTTON_B_GPIO           39

#define M5STICKC_PLUS_LCD_HOST                SPI2_HOST
#define M5STICKC_PLUS_LCD_PIXEL_CLOCK_HZ      (20 * 1000 * 1000)
#define M5STICKC_PLUS_LCD_H_RES               135
#define M5STICKC_PLUS_LCD_V_RES               240
#define M5STICKC_PLUS_LCD_GAP_X               52
#define M5STICKC_PLUS_LCD_GAP_Y               40
#define M5STICKC_PLUS_LCD_SCLK_GPIO           13
#define M5STICKC_PLUS_LCD_MOSI_GPIO           15
#define M5STICKC_PLUS_LCD_DC_GPIO             23
#define M5STICKC_PLUS_LCD_RST_GPIO            18
#define M5STICKC_PLUS_LCD_CS_GPIO             5

#define M5STICKC_PLUS_I2C_PORT                I2C_NUM_0
#define M5STICKC_PLUS_I2C_SDA_GPIO            21
#define M5STICKC_PLUS_I2C_SCL_GPIO            22
#define M5STICKC_PLUS_AXP192_I2C_ADDRESS      0x34
#define M5STICKC_PLUS_MPU6886_I2C_ADDRESS     0x68
