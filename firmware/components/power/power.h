#pragma once

#include <stdbool.h>

#include "esp_err.h"
#include "driver/gpio.h"
#include "hal/adc_types.h"

typedef struct {
    gpio_num_t battery_gpio;
    adc_channel_t battery_channel;
    gpio_num_t charge_pin;
    gpio_num_t charge_done_pin;
    gpio_num_t vbus_pin;
} power_config_t;

esp_err_t power_init(const power_config_t *cfg);
int power_battery_millivolts(void);
int power_battery_percent(void);
bool power_is_charging(void);
bool power_is_charging_done(void);
bool power_is_usb_powered(void);
