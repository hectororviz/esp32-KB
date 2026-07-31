#pragma once

#include "driver/gpio.h"

#define MATRIX_ROW_PINS    { GPIO_NUM_6, GPIO_NUM_7, GPIO_NUM_8, GPIO_NUM_9, GPIO_NUM_10 }
#define MATRIX_COL_PINS    { GPIO_NUM_11, GPIO_NUM_12, GPIO_NUM_13, GPIO_NUM_14 }

#define ENCODER_PIN_A      GPIO_NUM_15
#define ENCODER_PIN_B      GPIO_NUM_16
#define ENCODER_PIN_SW     GPIO_NUM_17

#define OLED_SDA           GPIO_NUM_4
#define OLED_SCL           GPIO_NUM_5
#define OLED_I2C_ADDR      0x3C

#define BATTERY_ADC_GPIO   GPIO_NUM_1
#define BATTERY_ADC_CH     ADC_CHANNEL_0
#define CHARGE_PIN         GPIO_NUM_2
#define CHARGE_DONE_PIN    GPIO_NUM_3
#define VBUS_PIN           GPIO_NUM_18
